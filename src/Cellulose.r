#include "AEConfig.h"
#include "AE_EffectVers.h"

#ifndef AE_OS_WIN
    #include "AE_General.r"
#endif

resource 'PiPL' (16000) {
    {
        Kind {
            AEEffect
        },
        Name {
            "Cellulose"
        },
        Category {
            "Cellulose"
        },
#ifdef AE_OS_WIN
    #if defined(AE_PROC_INTELx64)
        CodeWin64X86 { "EffectMain" },
    #elif defined(AE_PROC_ARM64)
        CodeWinARM64 { "EffectMain" },
    #endif
#elif defined(AE_OS_MAC)
        CodeMacIntel64 { "EffectMain" },
        CodeMacARM64   { "EffectMain" },
#endif
        AE_PiPL_Version {
            2, 0
        },
        AE_Effect_Spec_Version {
            PF_PLUG_IN_VERSION,
            PF_PLUG_IN_SUBVERS
        },
        AE_Effect_Version {
            0x00080001
        },
        AE_Effect_Info_Flags {
            0
        },
        // PF_OutFlag_DEEP_COLOR_AWARE | PF_OutFlag_USE_OUTPUT_EXTENT
        AE_Effect_Global_OutFlags {
            0x2000040
        },
        // PF_OutFlag2_SUPPORTS_SMART_RENDER | PF_OutFlag2_FLOAT_COLOR_AWARE |
        // PF_OutFlag2_PARAM_GROUP_START_COLLAPSED_FLAG
        AE_Effect_Global_OutFlags_2 {
            0x00001408
        },
        AE_Effect_Match_Name {
            "Cellulose"
        },
        AE_Reserved_Info {
            8
        }
    }
};
