# Cellulose — Celluloid Film Colour Oscillation Plugin

Cellulose is a cross-platform Adobe After Effects and Premiere Pro plugin that simulates the organic colour-boundary oscillation found in analogue celluloid film. Each colour boundary in your footage breathes and shifts independently, producing the characteristic instability of scanned film.

---

## Parameters

### Amplitude
**Range:** 0 – 500 px &nbsp;|&nbsp; **Default:** 10 px

Amplitude is the lung capacity of the effect — how far each displaced pixel can travel from its origin before the noise pulls it back.

At its quietest, near zero, the image barely stirs. Colour boundaries tremble in place like heat haze above summer asphalt, present only if you are looking for them. The footage remains entirely legible; reality holds.

As you push upward through the tens, the trembling becomes a sway. Edges drift. Faces retain their structure but their outlines breathe. The image starts to feel like something remembered rather than recorded.

Deeper still, into the hundreds, the effect crosses a threshold from texture into transformation. Whole regions of colour migrate away from their origins in long, sweeping arcs. The image begins to disagree with itself.

At the far extreme — four or five hundred pixels — coherence dissolves entirely. What was footage becomes a field of travelling colour, the original subject still faintly implied in the motion of its own remains. Whether this reads as destruction or as painting depends on what you brought to it.

---

### Frequency
**Range:** 0 – 60 Hz &nbsp;|&nbsp; **Default:** 3 Hz

Frequency governs how fast the noise field evolves through time — the metabolism of the effect, the rate at which it changes its mind.

At zero, time stops. The displacement field freezes at the noise configuration of the first frame and holds it for the entire duration of the clip. The image is warped, but consistently warped — deformed rather than animated, like a funhouse mirror rather than a living thing.

At low values — one or two hertz — the motion is slow and deliberate, the kind of drift you might attribute to a projector slightly out of phase with itself. The footage breathes at roughly the pace of a sleeping person.

Around the default of three hertz, the oscillation has a pulse. There is rhythm to it. It suggests something nervous but controlled — a hand steadied against a slight tremor.

As frequency climbs into the teens and twenties, the effect becomes a flicker. Individual frames diverge noticeably from their neighbours. The image seems unable to commit to a position, perpetually reconsidering.

At the upper extreme — forty, fifty, sixty hertz — the noise field rewrites itself many times per second. On screen this reads as a kind of visual static, a seething agitation that the eye cannot resolve into motion. The footage vibrates. It becomes anxious. It becomes a signal from somewhere much further away than where it was shot.

---

### Irregularity
**Range:** 0 – 1 &nbsp;|&nbsp; **Default:** 0.5

Irregularity controls the layering of the noise — specifically, how much weight is given to fine detail relative to broad movement. It is the difference between a single ocean swell and a sea covered in chop.

At zero, the displacement field is smooth and monolithic. A single broad wave moves across the image; everything it touches is pushed in roughly the same direction by roughly the same amount. The motion feels composed, almost architectural.

As the value rises, finer and finer layers of noise are folded into the field — each octave doubled in spatial frequency, riding on top of the one below. The motion becomes complex. No two adjacent pixels follow quite the same trajectory. The broad wave is still there beneath the surface, but you can no longer see it through the turbulence above.

At one, the texture of the displacement is as granular as the algorithm allows. Motion becomes intricate and contradictory — neighbouring pixels pulling in opposite directions, small vortices forming and dissolving within the larger flow. The result has the quality of something organic: a murmuration, a cellular process, water finding its way through gravel.

---

### Edge Sensitivity
**Range:** 0 – 2 &nbsp;|&nbsp; **Default:** 0.2

Edge Sensitivity decides which parts of the image are allowed to move and which must stay still. It is the selectivity of the effect's attention — its willingness to be disturbed.

At zero, the threshold is removed entirely. Every pixel in the frame participates. Flat areas of colour warp alongside the boundaries between them; the whole image shimmers as a single restless surface. There is no anchor. Nothing stays put.

Low values — around the default of 0.2 — admit most of the image into the effect while still concentrating the most intense displacement at the sharpest colour transitions. Broad gradients shift gently; hard edges shift violently. The footage feels globally unstable but locally coherent.

As sensitivity increases, the effect grows more discriminating. It begins to ignore soft gradients and low-contrast transitions, touching only the edges it considers truly significant. The still areas of the image hold perfectly; only the definite, declared boundaries between things animate. At high values this can produce a striking dissociation — a calm, frozen background behind edges that writhe independently, as though the outlines of objects have become autonomous from the objects themselves.

At one, the threshold is as high as it will go. Only the single sharpest edge in the entire frame — the one with the highest Sobel magnitude — meets the criterion. Everything else is stationary. The effect reduces to a surgical point.

---

### Colour Bleed
**Range:** 0 – 2 &nbsp;|&nbsp; **Default:** 0.2

Colour Bleed determines what happens in the neighbourhood of a displaced edge after it moves — whether the colours on either side of the boundary stay cleanly separated or begin to contaminate one another.

At zero, displacement is geometrically pure. Pixels move but they carry their own colour and nothing else. The result is crisp, precise: a warped image, but a sharply warped one. The edges, wherever they have landed, remain edges.

As the value rises, the effect begins to gather colour from the surrounding region and mix it into each displaced pixel proportionally to its proximity. Colours that were once separated by a boundary begin to find each other. Warm bleeds into cool. Light bleeds into shadow. The dyes of the film stock, if there were such a thing, are dissolving at their boundaries.

Around the middle of the range, the bleed creates a soft halation — a luminous interpenetration at every edge, as though the colours have been breathing on each other and exchanging warmth. It is the quality of old photographs where the inks have had decades to migrate through the paper.

At one, the bleed is total. Every displaced edge pixel becomes a weighted average of everything near it that also lives at an edge. The boundaries of things lose their sharpness and become zones of transition — not a line where blue meets red, but a region where each is becoming the other. Forms are still present, still implied, but their edges are now suggestions rather than declarations.

---

## Algorithm

Four sequential passes per frame:

1. **Edge Detection** — Sobel operator identifies colour boundaries across the frame, producing a normalised edge magnitude map
2. **Noise Field Generation** — Layered 3D simplex noise (X/Y = spatial, Z = time) produces organic, non-repeating displacement vectors with per-region phase offsets to break up tiling artefacts
3. **Boundary Displacement** — Per-pixel UV warping: pixels above the Edge Sensitivity threshold are displaced by noise vectors scaled by Amplitude and modulated by Frequency × time and Irregularity
4. **Colour Bleed** — Weighted neighbourhood average along displaced edges, radius proportional to Amplitude (capped at 30 px), blended by the Colour Bleed parameter

---

## GPU Acceleration

- **Windows / NVIDIA** — CUDA kernel (float32, one thread per pixel)
- **macOS** — Metal compute shader (same algorithm in MSL)
- **All platforms** — CPU fallback (8/16/32-bit, parallelised via `std::for_each`)

---

## Prerequisites

### All Platforms
- CMake 3.20 or newer
- Adobe After Effects SDK

### Windows
- Visual Studio 2019 or 2022 (MSVC) with C++17
- CUDA Toolkit 12.x (for GPU path; CPU-only builds work without it)

### macOS
- Xcode 15+ with Command Line Tools
- macOS 13 Ventura or newer

---

## Setup

### 1. Obtain the Adobe After Effects SDK

1. Sign in at [Adobe Developer](https://developer.adobe.com/after-effects/)
2. Download the **After Effects SDK** for your platform
3. Extract the archive and place the contents inside the `sdk/` directory of this project:

   ```
   Cellulose/sdk/
   ├── Headers/
   ├── Resources/
   ├── Examples/
   └── ...
   ```

### 2. Configure and Build

#### Windows (CPU only)
```bat
cmake -B build -DAESDK_ROOT=./sdk -DCELLULOSE_ENABLE_CUDA=OFF
cmake --build build --config Release
```

#### Windows (CUDA + CPU)
```bat
cmake -B build -DAESDK_ROOT=./sdk -DCELLULOSE_ENABLE_CUDA=ON
cmake --build build --config Release
```

#### macOS (Metal + CPU)
```bash
cmake -B build -DAESDK_ROOT=./sdk -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Output:
- Windows: `build/Release/Cellulose.aex`
- macOS: `build/Cellulose.plugin`

---

## Installation

### After Effects — Windows
Copy `Cellulose.aex` to:
```
C:\Program Files\Adobe\Adobe After Effects <version>\Support Files\Plug-ins\
```

### After Effects — macOS
Copy `Cellulose.plugin` to:
```
/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/
```

### Premiere Pro
The same `.aex` / `.plugin` works in Premiere Pro. On macOS, the shared MediaCore path above covers both applications. On Windows, copy to:
```
C:\Program Files\Adobe\Adobe Premiere Pro <version>\Plug-ins\Common\
```

---

## License

This plugin is provided as-is for personal and commercial use. The embedded simplex noise implementation is adapted from Stefan Gustavson's public-domain code.
