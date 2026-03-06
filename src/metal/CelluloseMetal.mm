// ---------------------------------------------------------------------------
// CelluloseMetal.mm
// Metal host: device selection, pipeline compilation, buffer management,
// and compute dispatch for the Cellulose effect.
//
// Compiled as Objective-C++ (ARC disabled for AE plugin compatibility).
// ---------------------------------------------------------------------------

#ifdef CELLULOSE_METAL_ENABLED

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

#include "CelluloseMetal.h"
#include "CelluloseAlgorithm.h"
#include "AE_Effect.h"
#include "AEFX_SuiteHelper.h"

// ---------------------------------------------------------------------------
// Uniform buffer layout — must match CelluloseMetal.metal exactly
// ---------------------------------------------------------------------------
struct MetalUniforms
{
    uint32_t width;
    uint32_t height;
    int32_t  currentFrame;
    float    amplitude;
    float    frequency;
    float    irregularity;
    float    edgeSensitivity;
    float    colourBleed;
    float    edgeBlur;
    float    canvasJitter;
    float    currentTime;
};

// ---------------------------------------------------------------------------
// Lazy pipeline cache — one pipeline per Metal device
// (AE may use multiple GPUs on a Mac Pro)
// ---------------------------------------------------------------------------
@interface CelluloseMetalState : NSObject
@property (nonatomic, strong) id<MTLDevice>              device;
@property (nonatomic, strong) id<MTLComputePipelineState> pipeline;
@property (nonatomic, strong) id<MTLCommandQueue>         commandQueue;
@end

@implementation CelluloseMetalState
@end

static NSMutableDictionary<NSValue*, CelluloseMetalState*>* s_pipelineCache = nil;
static dispatch_once_t s_cacheInit;

static CelluloseMetalState* GetOrCreateState(id<MTLDevice> device)
{
    dispatch_once(&s_cacheInit, ^{
        s_pipelineCache = [NSMutableDictionary new];
    });

    NSValue* key = [NSValue valueWithPointer:(__bridge void*)device];

    @synchronized(s_pipelineCache) {
        CelluloseMetalState* state = s_pipelineCache[key];
        if (state) return state;

        state = [CelluloseMetalState new];
        state.device = device;
        state.commandQueue = [device newCommandQueue];

        // Load the pre-compiled metallib embedded at build time
        NSError* err = nil;
        NSString* metalLibPath = @CELLULOSE_METALLIB_PATH;
        NSURL*    metalLibURL  = [NSURL fileURLWithPath:metalLibPath];
        id<MTLLibrary> lib = [device newLibraryWithURL:metalLibURL error:&err];

        if (!lib)
        {
            // Fall back: try compiling the source inline (development builds)
            NSLog(@"[Cellulose] Could not load metallib at %@: %@",
                  metalLibPath, err.localizedDescription);
            return nil;
        }

        id<MTLFunction> kernelFn = [lib newFunctionWithName:@"cellulose_main"];
        if (!kernelFn)
        {
            NSLog(@"[Cellulose] Metal function 'cellulose_main' not found in library");
            return nil;
        }

        id<MTLComputePipelineState> pipeline =
            [device newComputePipelineStateWithFunction:kernelFn error:&err];
        if (!pipeline)
        {
            NSLog(@"[Cellulose] Failed to create compute pipeline: %@",
                  err.localizedDescription);
            return nil;
        }

        state.pipeline = pipeline;
        s_pipelineCache[key] = state;
        return state;
    }
}

// ---------------------------------------------------------------------------
// Public render entry point
// ---------------------------------------------------------------------------
PF_Err CelluloseMetal::Render(
    PF_InData*             /*in_data*/,
    PF_OutData*            /*out_data*/,
    PF_EffectWorld*        input_world,
    PF_EffectWorld*        output_world,
    const CelluloseParams& cp,
    const void*            gpu_data,
    A_u_long               /*device_index*/)
{
    // Retrieve Metal device provided by AE via gpu_data
    id<MTLDevice> device = (__bridge id<MTLDevice>)gpu_data;
    if (!device) device = MTLCreateSystemDefaultDevice();
    if (!device) return PF_Err_INTERNAL_STRUCT_DAMAGED;

    CelluloseMetalState* state = GetOrCreateState(device);
    if (!state) return PF_Err_INTERNAL_STRUCT_DAMAGED;

    int W = cp.width, H = cp.height;
    size_t pixelBytes = (size_t)W * H * sizeof(float) * 4;  // float4 per pixel

    // Wrap AE-managed world data as Metal buffers
    id<MTLBuffer> srcBuf = [device newBufferWithBytesNoCopy:input_world->data
                                                     length:pixelBytes
                                                    options:MTLResourceStorageModeShared
                                                deallocator:nil];
    id<MTLBuffer> dstBuf = [device newBufferWithBytesNoCopy:output_world->data
                                                     length:pixelBytes
                                                    options:MTLResourceStorageModeShared
                                                deallocator:nil];

    // Build uniforms
    MetalUniforms uniforms;
    uniforms.width           = (uint32_t)W;
    uniforms.height          = (uint32_t)H;
    uniforms.currentFrame    = cp.currentFrame;
    uniforms.amplitude       = cp.amplitude;
    uniforms.frequency       = cp.frequency;
    uniforms.irregularity    = cp.irregularity;
    uniforms.edgeSensitivity = cp.edgeSensitivity;
    uniforms.colourBleed     = cp.colourBleed;
    uniforms.edgeBlur        = cp.edgeBlur;
    uniforms.canvasJitter    = cp.canvasJitter;
    uniforms.currentTime     = cp.currentTime;

    id<MTLBuffer> uniformBuf = [device newBufferWithBytes:&uniforms
                                                   length:sizeof(uniforms)
                                                  options:MTLResourceStorageModeShared];

    // Encode compute command
    id<MTLCommandBuffer>      cmdBuf  = [state.commandQueue commandBuffer];
    id<MTLComputeCommandEncoder> enc  = [cmdBuf computeCommandEncoder];

    [enc setComputePipelineState:state.pipeline];
    [enc setBuffer:srcBuf     offset:0 atIndex:0];
    [enc setBuffer:dstBuf     offset:0 atIndex:1];
    [enc setBuffer:uniformBuf offset:0 atIndex:2];

    // Dispatch threads: 16×16 threadgroup
    MTLSize threadsPerGroup = MTLSizeMake(16, 16, 1);
    MTLSize threadGroups    = MTLSizeMake(
        ((NSUInteger)W + 15) / 16,
        ((NSUInteger)H + 15) / 16,
        1);

    [enc dispatchThreadgroups:threadGroups threadsPerThreadgroup:threadsPerGroup];
    [enc endEncoding];

    [cmdBuf commit];
    [cmdBuf waitUntilCompleted];

    if (cmdBuf.status == MTLCommandBufferStatusError)
    {
        NSLog(@"[Cellulose] Metal command buffer error: %@",
              cmdBuf.error.localizedDescription);
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    return PF_Err_NONE;
}

#endif // CELLULOSE_METAL_ENABLED
