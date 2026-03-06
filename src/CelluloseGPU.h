#pragma once

// ---------------------------------------------------------------------------
// CelluloseGPU.h — Host-side GPU dispatch (CUDA on Windows, Metal on macOS).
// ---------------------------------------------------------------------------

#include "AE_Effect.h"
#include "CelluloseAlgorithm.h"

namespace CelluloseGPU
{
    // Called from Cellulose.cpp::SmartRender when what_gpu != PF_GPU_Framework_NONE.
    // Returns PF_Err_NONE on success; any other value signals fall-back to CPU.
    PF_Err Render(
        PF_InData*             in_data,
        PF_OutData*            out_data,
        PF_EffectWorld*        input_world,
        PF_EffectWorld*        output_world,
        const CelluloseParams& cellParams,
        PF_GPU_Framework       what_gpu,
        const void*            gpu_data,
        A_u_long               device_index);

} // namespace CelluloseGPU
