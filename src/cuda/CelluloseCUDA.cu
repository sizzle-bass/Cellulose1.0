// ---------------------------------------------------------------------------
// CelluloseCUDA.cu
// CUDA implementation of the Cellulose 4-pass celluloid oscillation effect.
//
// Passes (all within a single kernel launch):
//   1. Sobel edge detection  (uses shared memory tile)
//   2. 3D simplex noise field
//   3. UV displacement + bilinear sample
//   4. Colour bleed composite (second kernel pass)
//
// AE GPU path always provides float32 (RGBA float4) buffers.
// One CUDA thread per output pixel, launched as a 2D grid.
// ---------------------------------------------------------------------------

#include "CelluloseCUDA.cuh"
#include "CelluloseAlgorithm.h"

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <math_constants.h>

#include "AE_Effect.h"

// ---------------------------------------------------------------------------
// CUDA error checking helper
// ---------------------------------------------------------------------------
#define CUDA_CHECK(call) \
    do { \
        cudaError_t _err = (call); \
        if (_err != cudaSuccess) { \
            return PF_Err_INTERNAL_STRUCT_DAMAGED; \
        } \
    } while(0)

// ---------------------------------------------------------------------------
// Device: 3D Simplex Noise
// Stefan Gustavson — adapted for CUDA (no texture memory required)
// ---------------------------------------------------------------------------

__device__ static const int d_perm[512] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
    60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
    200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
    52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
    207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
    119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
    129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
    218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
    184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
    222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
    // doubled
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
    60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
    200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
    52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
    207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
    119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
    129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
    218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
    184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
    222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

__device__ static const int d_grad3[12][3] = {
    {1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},
    {1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},
    {0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1}
};

__device__ static float d_dot3(int gi, float x, float y, float z)
{
    return d_grad3[gi][0]*x + d_grad3[gi][1]*y + d_grad3[gi][2]*z;
}

__device__ static float d_snoise3(float xin, float yin, float zin)
{
    const float F3 = 0.333333333f;
    const float G3 = 0.166666667f;

    float s  = (xin + yin + zin) * F3;
    int i = __float2int_rd(xin + s);
    int j = __float2int_rd(yin + s);
    int k = __float2int_rd(zin + s);

    float t  = (i + j + k) * G3;
    float x0 = xin - (i - t);
    float y0 = yin - (j - t);
    float z0 = zin - (k - t);

    int i1, j1, k1, i2, j2, k2;
    if (x0 >= y0) {
        if      (y0 >= z0) { i1=1;j1=0;k1=0; i2=1;j2=1;k2=0; }
        else if (x0 >= z0) { i1=1;j1=0;k1=0; i2=1;j2=0;k2=1; }
        else               { i1=0;j1=0;k1=1; i2=1;j2=0;k2=1; }
    } else {
        if      (y0 < z0)  { i1=0;j1=0;k1=1; i2=0;j2=1;k2=1; }
        else if (x0 < z0)  { i1=0;j1=1;k1=0; i2=0;j2=1;k2=1; }
        else               { i1=0;j1=1;k1=0; i2=1;j2=1;k2=0; }
    }

    float x1 = x0 - i1 + G3,      y1 = y0 - j1 + G3,      z1 = z0 - k1 + G3;
    float x2 = x0 - i2 + 2.0f*G3, y2 = y0 - j2 + 2.0f*G3, z2 = z0 - k2 + 2.0f*G3;
    float x3 = x0 - 1.0f + 3.0f*G3, y3 = y0 - 1.0f + 3.0f*G3, z3 = z0 - 1.0f + 3.0f*G3;

    int ii = i & 255, jj = j & 255, kk = k & 255;
    int gi0 = d_perm[ii +     d_perm[jj +     d_perm[kk   ]]] % 12;
    int gi1 = d_perm[ii+i1 +  d_perm[jj+j1 +  d_perm[kk+k1]]] % 12;
    int gi2 = d_perm[ii+i2 +  d_perm[jj+j2 +  d_perm[kk+k2]]] % 12;
    int gi3 = d_perm[ii+1  +  d_perm[jj+1  +  d_perm[kk+1 ]]] % 12;

    auto contrib = [](int gi, float x, float y, float z) __device__ -> float {
        float t = 0.6f - x*x - y*y - z*z;
        if (t < 0.0f) return 0.0f;
        t *= t;
        return t * t * d_dot3(gi, x, y, z);
    };

    return 32.0f * (contrib(gi0,x0,y0,z0) + contrib(gi1,x1,y1,z1) +
                    contrib(gi2,x2,y2,z2) + contrib(gi3,x3,y3,z3));
}

__device__ static float d_layeredNoise(float x, float y, float z, float irregularity)
{
    float value = 0.0f, amplitude = 1.0f, frequency = 1.0f, maxVal = 0.0f;
    for (int oct = 0; oct < CELLULOSE_NOISE_OCTAVES; ++oct)
    {
        value    += d_snoise3(x*frequency, y*frequency, z*frequency) * amplitude;
        maxVal   += amplitude;
        amplitude *= irregularity;
        frequency *= 2.0f;
    }
    return (maxVal > 0.0f) ? value / maxVal : 0.0f;
}

// ---------------------------------------------------------------------------
// Device: bilinear sample from float4 buffer
// ---------------------------------------------------------------------------
__device__ static float4 d_bilinear(
    const float4* src, int W, int H, float u, float v)
{
    u = fmaxf(0.0f, fminf(u, (float)(W-1)));
    v = fmaxf(0.0f, fminf(v, (float)(H-1)));
    int x0 = (int)u, y0 = (int)v;
    int x1 = min(x0+1, W-1), y1 = min(y0+1, H-1);
    float tx = u - x0, ty = v - y0;

    float4 p00 = src[y0*W + x0];
    float4 p10 = src[y0*W + x1];
    float4 p01 = src[y1*W + x0];
    float4 p11 = src[y1*W + x1];

    auto lerp4 = [](float4 a, float4 b, float t) -> float4 {
        return { a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t,
                 a.z+(b.z-a.z)*t, a.w+(b.w-a.w)*t };
    };

    float4 lo = lerp4(p00, p10, tx);
    float4 hi = lerp4(p01, p11, tx);
    return lerp4(lo, hi, ty);
}

// ---------------------------------------------------------------------------
// Shared-memory tile dimensions for Sobel pass
// ---------------------------------------------------------------------------
#define TILE_W 18   // 16 + 2 halo pixels
#define TILE_H 18

// ---------------------------------------------------------------------------
// Main CUDA kernel — one thread per output pixel
// ---------------------------------------------------------------------------
__global__ void CelluloseKernel(
    const float4* __restrict__ src,
          float4* __restrict__ dst,
    int   W, int H,
    float amplitude,
    float frequency,
    float irregularity,
    float edgeSensitivity,
    float colourBleed,
    float edgeBlur,
    float currentTime,
    float jitterX,
    float jitterY)
{
    // Thread coordinates
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    // ---------- Pass 1: Sobel via shared memory tile ----------
    __shared__ float tile[TILE_H][TILE_W];

    int lx = threadIdx.x + 1;  // local x with 1-pixel halo
    int ly = threadIdx.y + 1;

    // Load centre pixel luminance into shared memory
    auto luminance = [](float4 p) { return 0.299f*p.x + 0.587f*p.y + 0.114f*p.z; };

    auto clampSample = [&](int cx, int cy) -> float {
        cx = max(0, min(cx, W-1));
        cy = max(0, min(cy, H-1));
        return luminance(src[cy*W + cx]);
    };

    if (x < W && y < H) tile[ly][lx] = luminance(src[y*W + x]);

    // Halo pixels
    if (threadIdx.x == 0)              tile[ly][0]       = clampSample(x-1, y);
    if (threadIdx.x == blockDim.x-1)   tile[ly][TILE_W-1]= clampSample(x+1, y);
    if (threadIdx.y == 0)              tile[0][lx]        = clampSample(x, y-1);
    if (threadIdx.y == blockDim.y-1)   tile[TILE_H-1][lx] = clampSample(x, y+1);
    // Corners
    if (threadIdx.x == 0 && threadIdx.y == 0)
        tile[0][0] = clampSample(x-1, y-1);
    if (threadIdx.x == blockDim.x-1 && threadIdx.y == 0)
        tile[0][TILE_W-1] = clampSample(x+1, y-1);
    if (threadIdx.x == 0 && threadIdx.y == blockDim.y-1)
        tile[TILE_H-1][0] = clampSample(x-1, y+1);
    if (threadIdx.x == blockDim.x-1 && threadIdx.y == blockDim.y-1)
        tile[TILE_H-1][TILE_W-1] = clampSample(x+1, y+1);

    __syncthreads();

    if (x >= W || y >= H) return;

    float tl = tile[ly-1][lx-1], tc = tile[ly-1][lx], tr = tile[ly-1][lx+1];
    float ml = tile[ly  ][lx-1],                        mr = tile[ly  ][lx+1];
    float bl = tile[ly+1][lx-1], bc = tile[ly+1][lx], br = tile[ly+1][lx+1];

    float gx = -tl - 2.0f*ml - bl + tr + 2.0f*mr + br;
    float gy = -tl - 2.0f*tc - tr + bl + 2.0f*bc + br;
    // Normalise Sobel magnitude to [0,1] — divide by theoretical max (4*1+2*2=8... ~4.0 works)
    float edge = sqrtf(gx*gx + gy*gy) * 0.25f;
    edge = fminf(edge, 1.0f);

    float srcU = (float)x + jitterX;
    float srcV = (float)y + jitterY;

    // ---------- Passes 2 & 3: noise + displacement ----------
    if (edge > edgeSensitivity)
    {
        float invW = 1.0f / W, invH = 1.0f / H;
        int zoneX = x / CELLULOSE_ZONE_SIZE, zoneY = y / CELLULOSE_ZONE_SIZE;
        float phaseX = (float)(zoneX * 127 + zoneY * 311) * 0.01f;
        float phaseY = (float)(zoneX * 211 + zoneY * 97 ) * 0.01f;

        float nx = (float)x * invW + phaseX;
        float ny = (float)y * invH + phaseY;
        float nt = currentTime * frequency;

        float dx = d_layeredNoise(nx,         ny + 17.3f, nt,        irregularity);
        float dy = d_layeredNoise(nx + 31.7f, ny,         nt + 5.1f, irregularity);

        float strength = (edge - edgeSensitivity) / (1.0f - edgeSensitivity + 1e-6f);
        float disp = amplitude * strength;

        srcU = fmaxf(0.0f, fminf(srcU + dx * disp, (float)(W-1)));
        srcV = fmaxf(0.0f, fminf(srcV + dy * disp, (float)(H-1)));
    }

    // Bilinear sample
    float4 out = d_bilinear(src, W, H, srcU, srcV);

    // ---------- Pass 4: colour bleed ----------
    if (edge > edgeSensitivity && colourBleed > 0.0f)
    {
        int radius = max(1, (int)(amplitude * 0.5f));
        radius = min(radius, 4);  // cap for performance

        float sumR=0, sumG=0, sumB=0, sumA=0, weight=0;
        for (int dy = -radius; dy <= radius; ++dy)
        {
            int sy = min(max(y + dy, 0), H-1);
            for (int dx2 = -radius; dx2 <= radius; ++dx2)
            {
                int sx = min(max(x + dx2, 0), W-1);
                // Approximate edge at neighbour from shared tile (if in tile)
                float4 neighbour = src[sy*W + sx];
                float w = 1.0f / (1.0f + sqrtf((float)(dx2*dx2 + dy*dy)));
                sumR += neighbour.x * w;
                sumG += neighbour.y * w;
                sumB += neighbour.z * w;
                sumA += neighbour.w * w;
                weight += w;
            }
        }
        if (weight > 0.0f)
        {
            float b = colourBleed;
            out.x = out.x*(1-b) + (sumR/weight)*b;
            out.y = out.y*(1-b) + (sumG/weight)*b;
            out.z = out.z*(1-b) + (sumB/weight)*b;
            out.w = out.w*(1-b) + (sumA/weight)*b;
        }
    }

    // ---------- Pass 5: edge blur ----------
    if (edge > edgeSensitivity && edgeBlur > 0.0f)
    {
        float strength = (edge - edgeSensitivity) / (1.0f - edgeSensitivity + 1e-6f);
        int   radius   = max(1, (int)(edgeBlur * 8.0f + 0.5f));
        int   r2       = radius * radius;

        float sumR=0, sumG=0, sumB=0, sumA=0, totalW=0;
        for (int ky = -radius; ky <= radius; ++ky)
        {
            for (int kx = -radius; kx <= radius; ++kx)
            {
                if (kx*kx + ky*ky > r2) continue;
                float4 s = d_bilinear(src, W, H,
                    fmaxf(0.0f, fminf(srcU + kx, (float)(W-1))),
                    fmaxf(0.0f, fminf(srcV + ky, (float)(H-1))));
                sumR += s.x; sumG += s.y; sumB += s.z; sumA += s.w;
                totalW += 1.0f;
            }
        }
        if (totalW > 0.0f)
        {
            float blend = strength * edgeBlur;
            out.x = out.x*(1-blend) + (sumR/totalW)*blend;
            out.y = out.y*(1-blend) + (sumG/totalW)*blend;
            out.z = out.z*(1-blend) + (sumB/totalW)*blend;
            out.w = out.w*(1-blend) + (sumA/totalW)*blend;
        }
    }

    dst[y*W + x] = out;
}

// ---------------------------------------------------------------------------
// Host entry point
// ---------------------------------------------------------------------------
PF_Err CelluloseCUDA::Render(
    PF_InData*             in_data,
    PF_OutData*            /*out_data*/,
    PF_EffectWorld*        input_world,
    PF_EffectWorld*        output_world,
    const CelluloseParams& cp,
    const void*            /*gpu_data*/,
    A_u_long               /*device_index*/)
{
    int W = cp.width, H = cp.height;

    // Pre-compute per-frame canvas jitter on the host (same value for all threads)
    float jitterX = 0.0f, jitterY = 0.0f;
    if (cp.canvasJitter > 0.0f)
    {
        uint32_t h = static_cast<uint32_t>(cp.currentFrame) * 2654435761u;
        h ^= h >> 16;
        h *= 0x45d9f3bu;
        h ^= h >> 16;
        jitterX = (static_cast<float>(h >> 16) / 32767.5f - 1.0f) * cp.canvasJitter;
        h *= 2246822519u;
        h ^= h >> 13;
        jitterY = (static_cast<float>(h >> 16) / 32767.5f - 1.0f) * cp.canvasJitter * 0.15f;
    }

    // When AE calls with CUDA framework, world->data contains CUDA device pointers
    float4* d_src = reinterpret_cast<float4*>(input_world->data);
    float4* d_dst = reinterpret_cast<float4*>(output_world->data);

    dim3 blockDim(16, 16);
    dim3 gridDim(
        (W + blockDim.x - 1) / blockDim.x,
        (H + blockDim.y - 1) / blockDim.y);

    CelluloseKernel<<<gridDim, blockDim>>>(
        d_src, d_dst,
        W, H,
        cp.amplitude, cp.frequency, cp.irregularity,
        cp.edgeSensitivity, cp.colourBleed, cp.edgeBlur,
        cp.currentTime, jitterX, jitterY);

    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    return PF_Err_NONE;
}
