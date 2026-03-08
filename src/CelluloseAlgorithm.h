#pragma once

// ---------------------------------------------------------------------------
// CelluloseAlgorithm.h
// Shared algorithm types visible to both CPU and GPU (CUDA/Metal) code.
// Keep this header free of platform-specific or SDK-specific includes so it
// can be included directly in .cu and .metal translation units.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Parameter block passed to every render tier
// ---------------------------------------------------------------------------
struct CelluloseParams
{
    float amplitude;         // maximum displacement, pixels
    float frequency;         // oscillation speed, Hz
    float irregularity;      // noise octave layering [0..1]
    float edgeSensitivity;   // Sobel threshold      [0..1]
    float colourBleed;       // colour mixing        [0..1]
    float canvasJitter;      // whole-frame gate jitter, pixels
    float edgeBlur;          // defocus blur strength along edges [0..1]
    float crushBlacks;       // shadow deepening post-process    [0..2]
    float vibrance;          // saturation boost                 [0..2]
    float vibranceFocus;     // protect vivid colours from boost [0..1]
    float currentTime;       // composition time, seconds
    int   currentFrame;      // frame index (current_time / time_step)
    int   bitDepth;          // 8, 16, or 32 (GPU path always 32)
    int   width;             // frame width  in pixels
    int   height;            // frame height in pixels
};

// ---------------------------------------------------------------------------
// Simplex noise constants shared between CPU and CUDA implementations.
// (Metal uses its own inline version due to MSL constraints.)
// ---------------------------------------------------------------------------

// Skewing / unskewing factors for 3D simplex noise
// F3 = 1/3,  G3 = 1/6
#ifndef CELLULOSE_METAL_SHADER   // Metal uses 'constant' qualifier; skip here
static constexpr float SIMPLEX_F3 = 0.333333333f;
static constexpr float SIMPLEX_G3 = 0.166666667f;
#endif

// Number of noise octaves used in the layered field
static constexpr int CELLULOSE_NOISE_OCTAVES = 3;

// Size of the independent-phase zones (pixels)
// Each zone gets a unique seed offset so adjacent regions oscillate differently
static constexpr int CELLULOSE_ZONE_SIZE = 32;
