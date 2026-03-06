#pragma once

// ---------------------------------------------------------------------------
// CelluloseCUDA.cuh — CUDA device declarations.
// ---------------------------------------------------------------------------

#include "AE_Effect.h"
#include "CelluloseAlgorithm.h"

namespace CelluloseCUDA
{
    PF_Err Render(
        PF_InData*             in_data,
        PF_OutData*            out_data,
        PF_EffectWorld*        input_world,
        PF_EffectWorld*        output_world,
        const CelluloseParams& cellParams,
        const void*            gpu_data,
        A_u_long               device_index);

} // namespace CelluloseCUDA

#ifdef __CUDACC__
__global__ void CelluloseKernel(
    const float4* __restrict__ src,
          float4* __restrict__ dst,
    int   width, int height,
    float amplitude, float frequency, float irregularity,
    float edgeSensitivity, float colourBleed, float currentTime);
#endif
