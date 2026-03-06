#pragma once

// ---------------------------------------------------------------------------
// Cellulose.h — Plugin constants, version info, and SDK include wrapper.
// ---------------------------------------------------------------------------

// --- AE SDK headers (Headers/ directory) -----------------------------------
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_EffectCBSuites.h"
#include "AE_Macros.h"
#include "AEConfig.h"
#include "AE_EffectVers.h"

// --- AE SDK utility headers (Util/ directory) ------------------------------
#include "entry.h"          // DllExport
#include "AEFX_SuiteHelper.h"
#include "Param_Utils.h"    // PF_ADD_FLOAT_SLIDERX
#include "Smart_Utils.h"

// --- Standard library ------------------------------------------------------
#include <cmath>
#include <algorithm>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Plugin identity
// ---------------------------------------------------------------------------
#define CELLULOSE_NAME        "Cellulose"
#define CELLULOSE_DESCRIPTION "Organic celluloid colour-boundary oscillation"
#define CELLULOSE_MATCH_NAME  "Cellulose"

#define CELLULOSE_MAJOR_VERSION  1
#define CELLULOSE_MINOR_VERSION  0
#define CELLULOSE_BUG_VERSION    0
#define CELLULOSE_STAGE_VERSION  PF_Stage_DEVELOP
#define CELLULOSE_BUILD_VERSION  1

#define CELLULOSE_VERSION \
    PF_VERSION(CELLULOSE_MAJOR_VERSION, \
               CELLULOSE_MINOR_VERSION, \
               CELLULOSE_BUG_VERSION,   \
               CELLULOSE_STAGE_VERSION, \
               CELLULOSE_BUILD_VERSION)

// ---------------------------------------------------------------------------
// Export declaration
// ---------------------------------------------------------------------------
extern "C"
{
    DllExport PF_Err EffectMain(
        PF_Cmd       cmd,
        PF_InData*   in_data,
        PF_OutData*  out_data,
        PF_ParamDef* params[],
        PF_LayerDef* output,
        void*        extra);
}

// ---------------------------------------------------------------------------
// Utility: convert AE fixed-point time to seconds
// ---------------------------------------------------------------------------
inline float AETimeToSeconds(const PF_InData* in_data)
{
    if (in_data->time_scale == 0) return 0.0f;
    return static_cast<float>(in_data->current_time) /
           static_cast<float>(in_data->time_scale);
}
