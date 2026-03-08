#pragma once

// ---------------------------------------------------------------------------
// CelluloseParams.h
// Parameter IDs, UI strings, ranges, and defaults for the Cellulose plugin.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Parameter indices
// AE parameter arrays are 0-indexed; index 0 is always the input layer.
// ---------------------------------------------------------------------------
enum ParamID
{
    PARAM_INPUT           = 0,  // implicit input layer (do not add via PF_ADD_*)
    PARAM_AMPLITUDE       = 1,  // max displacement in pixels
    PARAM_FREQUENCY       = 2,  // oscillation speed (Hz)
    PARAM_IRREGULARITY    = 3,  // noise octave blend
    PARAM_EDGE_SENSITIVITY= 4,  // Sobel threshold
    PARAM_COLOUR_BLEED    = 5,  // cross-boundary colour mixing
    PARAM_CANVAS_JITTER   = 6,  // whole-frame mechanical gate jitter
    PARAM_EDGE_BLUR       = 7,  // defocus blur along object boundaries
    PARAM_CRUSH_BLACKS    = 8,  // shadow deepening post-process
    PARAM_VIBRANCE        = 9,  // saturation boost to counter milky/grey cast
    PARAM_VIBRANCE_FOCUS  = 10, // focus vibrance on low-saturation colours only

    PARAM_COUNT                 // total number of parameters (including input)
};

// ---------------------------------------------------------------------------
// UI strings
// ---------------------------------------------------------------------------
#define CELLULOSE_PARAM_AMPLITUDE_NAME        "Amplitude"
#define CELLULOSE_PARAM_FREQUENCY_NAME        "Frequency"
#define CELLULOSE_PARAM_IRREGULARITY_NAME     "Irregularity"
#define CELLULOSE_PARAM_EDGE_SENSITIVITY_NAME "Edge Sensitivity"
#define CELLULOSE_PARAM_COLOUR_BLEED_NAME     "Colour Bleed"
#define CELLULOSE_PARAM_CANVAS_JITTER_NAME    "Canvas Jitter"
#define CELLULOSE_PARAM_EDGE_BLUR_NAME        "Edge Blur"
#define CELLULOSE_PARAM_CRUSH_BLACKS_NAME     "Crush Blacks"
#define CELLULOSE_PARAM_VIBRANCE_NAME         "Vibrance"
#define CELLULOSE_PARAM_VIBRANCE_FOCUS_NAME   "Vibrance Focus"

// ---------------------------------------------------------------------------
// Ranges and defaults (all float sliders)
// ---------------------------------------------------------------------------

// Amplitude: pixels of maximum boundary displacement
#define CELLULOSE_AMPLITUDE_MIN      0.0f
#define CELLULOSE_AMPLITUDE_MAX    100.0f
#define CELLULOSE_AMPLITUDE_DEFAULT 0.02f

// Frequency: oscillation cycles per second (0 = frozen/static noise field)
#define CELLULOSE_FREQUENCY_MIN      0.0f
#define CELLULOSE_FREQUENCY_MAX     12.0f
#define CELLULOSE_FREQUENCY_DEFAULT  0.10f

// Irregularity: blend between smooth (0) and turbulent (1) noise
#define CELLULOSE_IRREGULARITY_MIN      0.0f
#define CELLULOSE_IRREGULARITY_MAX      0.2f
#define CELLULOSE_IRREGULARITY_DEFAULT  0.04f

// Edge Sensitivity: Sobel magnitude threshold [-10 = force all pixels, 0 = detect everything, 1 = only strong edges]
#define CELLULOSE_EDGE_SENSITIVITY_MIN      -10.0f
#define CELLULOSE_EDGE_SENSITIVITY_MAX      2.0f
#define CELLULOSE_EDGE_SENSITIVITY_DEFAULT  -2.51f

// Colour Bleed: neighbourhood colour mixing strength
#define CELLULOSE_COLOUR_BLEED_MIN      0.0f
#define CELLULOSE_COLOUR_BLEED_MAX      2.0f
#define CELLULOSE_COLOUR_BLEED_DEFAULT  0.58f

// Canvas Jitter: maximum whole-frame gate displacement in pixels
// 0 = imperceptible, 10 = severe 1920s-era projector instability
#define CELLULOSE_CANVAS_JITTER_MIN      0.0f
#define CELLULOSE_CANVAS_JITTER_MAX     10.0f
#define CELLULOSE_CANVAS_JITTER_DEFAULT  1.75f

// Edge Blur: circular defocus blur radius along object boundaries
// 0 = no blur, 2 = maximum softness (~16px radius)
#define CELLULOSE_EDGE_BLUR_MIN      0.0f
#define CELLULOSE_EDGE_BLUR_MAX      2.0f
#define CELLULOSE_EDGE_BLUR_DEFAULT  0.39f

// Crush Blacks: shadow deepening — darkens near-black pixels, leaves highlights untouched
// 0 = off, 2 = maximum crush
#define CELLULOSE_CRUSH_BLACKS_MIN      0.0f
#define CELLULOSE_CRUSH_BLACKS_MAX      2.0f
#define CELLULOSE_CRUSH_BLACKS_DEFAULT  0.43f

// Vibrance: saturation multiplier to counter milky/grey cast introduced by other passes
// 0 = no boost, 2 = 3x chroma amplification
#define CELLULOSE_VIBRANCE_MIN      0.0f
#define CELLULOSE_VIBRANCE_MAX      2.0f
#define CELLULOSE_VIBRANCE_DEFAULT  0.68f

// Vibrance Focus: how much to protect already-saturated colours from the boost
// 0 = boost all colours equally, 1 = boost only dull/low-saturation colours
#define CELLULOSE_VIBRANCE_FOCUS_MIN      0.0f
#define CELLULOSE_VIBRANCE_FOCUS_MAX      1.0f
#define CELLULOSE_VIBRANCE_FOCUS_DEFAULT  1.00f
