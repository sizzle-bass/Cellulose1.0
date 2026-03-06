#pragma once

// ---------------------------------------------------------------------------
// CelluloseCPU.h
// CPU fallback render path — 8, 16, and 32-bit per-pixel.
// ---------------------------------------------------------------------------

#include "AE_Effect.h"
#include "CelluloseAlgorithm.h"

namespace CelluloseCPU
{
    // Main entry point called from Cellulose.cpp
    PF_Err Render(
        PF_InData*      in_data,
        PF_EffectWorld* input,
        PF_EffectWorld* output,
        const CelluloseParams& params);

} // namespace CelluloseCPU
