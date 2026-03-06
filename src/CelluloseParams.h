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

// ---------------------------------------------------------------------------
// Ranges and defaults (all float sliders)
// ---------------------------------------------------------------------------

// Amplitude: pixels of maximum boundary displacement
#define CELLULOSE_AMPLITUDE_MIN      0.0f
#define CELLULOSE_AMPLITUDE_MAX    100.0f
#define CELLULOSE_AMPLITUDE_DEFAULT 10.0f

// Frequency: oscillation cycles per second (0 = frozen/static noise field)
#define CELLULOSE_FREQUENCY_MIN      0.0f
#define CELLULOSE_FREQUENCY_MAX     12.0f
#define CELLULOSE_FREQUENCY_DEFAULT  3.0f

// Irregularity: blend between smooth (0) and turbulent (1) noise
#define CELLULOSE_IRREGULARITY_MIN      0.0f
#define CELLULOSE_IRREGULARITY_MAX      0.2f
#define CELLULOSE_IRREGULARITY_DEFAULT  0.2f

// Edge Sensitivity: Sobel magnitude threshold [0 = detect everything, 1 = only strong edges]
#define CELLULOSE_EDGE_SENSITIVITY_MIN      0.0f
#define CELLULOSE_EDGE_SENSITIVITY_MAX      2.0f
#define CELLULOSE_EDGE_SENSITIVITY_DEFAULT  0.2f

// Colour Bleed: neighbourhood colour mixing strength
#define CELLULOSE_COLOUR_BLEED_MIN      0.0f
#define CELLULOSE_COLOUR_BLEED_MAX      2.0f
#define CELLULOSE_COLOUR_BLEED_DEFAULT  0.2f

// Canvas Jitter: maximum whole-frame gate displacement in pixels
// 0 = imperceptible, 10 = severe 1920s-era projector instability
#define CELLULOSE_CANVAS_JITTER_MIN      0.0f
#define CELLULOSE_CANVAS_JITTER_MAX     10.0f
#define CELLULOSE_CANVAS_JITTER_DEFAULT  0.0f

// Edge Blur: circular defocus blur radius along object boundaries
// 0 = no blur, 2 = maximum softness (~16px radius)
#define CELLULOSE_EDGE_BLUR_MIN      0.0f
#define CELLULOSE_EDGE_BLUR_MAX      2.0f
#define CELLULOSE_EDGE_BLUR_DEFAULT  0.0f
