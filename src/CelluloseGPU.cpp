// ---------------------------------------------------------------------------
// CelluloseGPU.cpp — Host-side GPU render dispatcher.
// Selects CUDA (Windows/NVIDIA) or Metal (macOS) based on what_gpu.
// Returns an error if no matching GPU path is compiled in, causing CPU fallback.
// ---------------------------------------------------------------------------

#include "CelluloseGPU.h"

#ifdef CELLULOSE_CUDA_ENABLED
#  include "CelluloseCUDA.cuh"
#endif

#ifdef CELLULOSE_METAL_ENABLED
#  include "CelluloseMetal.h"
#endif

PF_Err CelluloseGPU::Render(
    PF_InData*             in_data,
    PF_OutData*            out_data,
    PF_EffectWorld*        input_world,
    PF_EffectWorld*        output_world,
    const CelluloseParams& cellParams,
    PF_GPU_Framework       what_gpu,
    const void*            gpu_data,
    A_u_long               device_index)
{
#ifdef CELLULOSE_CUDA_ENABLED
    if (what_gpu == PF_GPU_Framework_CUDA)
    {
        return CelluloseCUDA::Render(
            in_data, out_data, input_world, output_world,
            cellParams, gpu_data, device_index);
    }
#endif

#ifdef CELLULOSE_METAL_ENABLED
    if (what_gpu == PF_GPU_Framework_METAL)
    {
        return CelluloseMetal::Render(
            in_data, out_data, input_world, output_world,
            cellParams, gpu_data, device_index);
    }
#endif

    // No matching GPU path — caller will fall back to CPU
    return PF_Err_UNRECOGNIZED_PARAM_TYPE;
}
