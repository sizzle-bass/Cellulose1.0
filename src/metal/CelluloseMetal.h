#pragma once

// ---------------------------------------------------------------------------
// CelluloseMetal.h — Metal host-side declarations (pure C++ header).
// ---------------------------------------------------------------------------

#include "AE_Effect.h"
#include "CelluloseAlgorithm.h"

namespace CelluloseMetal
{
    PF_Err Render(
        PF_InData*             in_data,
        PF_OutData*            out_data,
        PF_EffectWorld*        input_world,
        PF_EffectWorld*        output_world,
        const CelluloseParams& cellParams,
        const void*            gpu_data,
        A_u_long               device_index);

} // namespace CelluloseMetal
