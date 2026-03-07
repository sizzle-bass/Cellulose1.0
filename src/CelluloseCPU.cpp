// ---------------------------------------------------------------------------
// CelluloseCPU.cpp
// CPU fallback: 4-pass celluloid oscillation.
//   Pass 1 — Sobel edge detection
//   Pass 2 — 3D simplex noise field (layered octaves)
//   Pass 3 — UV displacement + bilinear sample
//   Pass 4 — Colour bleed composite
//
// Supports 8-bit (PF_Pixel8), 16-bit (PF_Pixel16), 32-bit (PF_PixelFloat).
// Parallelised via std::for_each with std::execution::par_unseq where available.
// ---------------------------------------------------------------------------

#include "CelluloseCPU.h"
#include "CelluloseAlgorithm.h"

#include <vector>
#include <numeric>
#include <cmath>
#include <algorithm>
#include <execution>


// ---------------------------------------------------------------------------
// 3D Simplex Noise — Stefan Gustavson, public domain
// Adapted for plain C++ (no GLSL built-ins).
// ---------------------------------------------------------------------------
namespace
{

// Gradient table (12 edges of a cube)
static const int GRAD3[12][3] = {
    {1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},
    {1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},
    {0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1}
};

// Permutation table — standard Gustavson values, doubled, compile-time constant.
// Being constexpr means no runtime initialisation and no thread-safety concerns.
static constexpr int PERM[512] = {
    151,160,137, 91, 90, 15,131, 13,201, 95, 96, 53,194,233,  7,225,
    140, 36,103, 30, 69,142,  8, 99, 37,240, 21, 10, 23,190,  6,148,
    247,120,234, 75,  0, 26,197, 62, 94,252,219,203,117, 35, 11, 32,
     57,177, 33, 88,237,149, 56, 87,174, 20,125,136,171,168, 68,175,
     74,165, 71,134,139, 48, 27,166, 77,146,158,231, 83,111,229,122,
     60,211,133,230,220,105, 92, 41, 55, 46,245, 40,244,102,143, 54,
     65, 25, 63,161,  1,216, 80, 73,209, 76,132,187,208, 89, 18,169,
    200,196,135,130,116,188,159, 86,164,100,109,198,173,186,  3, 64,
     52,217,226,250,124,123,  5,202, 38,147,118,126,255, 82, 85,212,
    207,206, 59,227, 47, 16, 58, 17,182,189, 28, 42,223,183,170,213,
    119,248,152,  2, 44,154,163, 70,221,153,101,155,167, 43,172,  9,
    129, 22, 39,253, 19, 98,108,110, 79,113,224,232,178,185,112,104,
    218,246, 97,228,251, 34,242,193,238,210,144, 12,191,179,162,241,
     81, 51,145,235,249, 14,239,107, 49,192,214, 31,181,199,106,157,
    184, 84,204,176,115,121, 50, 45,127,  4,150,254,138,236,205, 93,
    222,114, 67, 29, 24, 72,243,141,128,195, 78, 66,215, 61,156,180,
    // second half (duplicate)
    151,160,137, 91, 90, 15,131, 13,201, 95, 96, 53,194,233,  7,225,
    140, 36,103, 30, 69,142,  8, 99, 37,240, 21, 10, 23,190,  6,148,
    247,120,234, 75,  0, 26,197, 62, 94,252,219,203,117, 35, 11, 32,
     57,177, 33, 88,237,149, 56, 87,174, 20,125,136,171,168, 68,175,
     74,165, 71,134,139, 48, 27,166, 77,146,158,231, 83,111,229,122,
     60,211,133,230,220,105, 92, 41, 55, 46,245, 40,244,102,143, 54,
     65, 25, 63,161,  1,216, 80, 73,209, 76,132,187,208, 89, 18,169,
    200,196,135,130,116,188,159, 86,164,100,109,198,173,186,  3, 64,
     52,217,226,250,124,123,  5,202, 38,147,118,126,255, 82, 85,212,
    207,206, 59,227, 47, 16, 58, 17,182,189, 28, 42,223,183,170,213,
    119,248,152,  2, 44,154,163, 70,221,153,101,155,167, 43,172,  9,
    129, 22, 39,253, 19, 98,108,110, 79,113,224,232,178,185,112,104,
    218,246, 97,228,251, 34,242,193,238,210,144, 12,191,179,162,241,
     81, 51,145,235,249, 14,239,107, 49,192,214, 31,181,199,106,157,
    184, 84,204,176,115,121, 50, 45,127,  4,150,254,138,236,205, 93,
    222,114, 67, 29, 24, 72,243,141,128,195, 78, 66,215, 61,156,180
};

static inline float Dot3(const int g[3], float x, float y, float z)
{
    return g[0]*x + g[1]*y + g[2]*z;
}

// Returns simplex noise in [-1, 1]
static float Snoise3(float xin, float yin, float zin)
{
    // Skew input to simplex cell
    float s = (xin + yin + zin) * SIMPLEX_F3;
    int i = (int)std::floor(xin + s);
    int j = (int)std::floor(yin + s);
    int k = (int)std::floor(zin + s);

    float t = (i + j + k) * SIMPLEX_G3;
    float x0 = xin - (i - t);
    float y0 = yin - (j - t);
    float z0 = zin - (k - t);

    // Determine simplex corner offsets
    int i1, j1, k1, i2, j2, k2;
    if (x0 >= y0) {
        if      (y0 >= z0) { i1=1; j1=0; k1=0; i2=1; j2=1; k2=0; }
        else if (x0 >= z0) { i1=1; j1=0; k1=0; i2=1; j2=0; k2=1; }
        else               { i1=0; j1=0; k1=1; i2=1; j2=0; k2=1; }
    } else {
        if      (y0 < z0)  { i1=0; j1=0; k1=1; i2=0; j2=1; k2=1; }
        else if (x0 < z0)  { i1=0; j1=1; k1=0; i2=0; j2=1; k2=1; }
        else               { i1=0; j1=1; k1=0; i2=1; j2=1; k2=0; }
    }

    float x1 = x0 - i1 + SIMPLEX_G3;
    float y1 = y0 - j1 + SIMPLEX_G3;
    float z1 = z0 - k1 + SIMPLEX_G3;
    float x2 = x0 - i2 + 2.0f*SIMPLEX_G3;
    float y2 = y0 - j2 + 2.0f*SIMPLEX_G3;
    float z2 = z0 - k2 + 2.0f*SIMPLEX_G3;
    float x3 = x0 - 1.0f + 3.0f*SIMPLEX_G3;
    float y3 = y0 - 1.0f + 3.0f*SIMPLEX_G3;
    float z3 = z0 - 1.0f + 3.0f*SIMPLEX_G3;

    int ii = i & 255, jj = j & 255, kk = k & 255;
    int gi0 = PERM[ii + PERM[jj + PERM[kk]]] % 12;
    int gi1 = PERM[ii+i1 + PERM[jj+j1 + PERM[kk+k1]]] % 12;
    int gi2 = PERM[ii+i2 + PERM[jj+j2 + PERM[kk+k2]]] % 12;
    int gi3 = PERM[ii+1  + PERM[jj+1  + PERM[kk+1 ]]] % 12;

    auto contrib = [](const int g[3], float x, float y, float z) -> float {
        float t = 0.6f - x*x - y*y - z*z;
        if (t < 0.0f) return 0.0f;
        t *= t;
        return t * t * Dot3(g, x, y, z);
    };

    float n = 32.0f * (
        contrib(GRAD3[gi0], x0, y0, z0) +
        contrib(GRAD3[gi1], x1, y1, z1) +
        contrib(GRAD3[gi2], x2, y2, z2) +
        contrib(GRAD3[gi3], x3, y3, z3));
    return n; // [-1..1]
}

// Layered (fractal) noise: CELLULOSE_NOISE_OCTAVES octaves
static float LayeredNoise(float x, float y, float z, float irregularity)
{
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float max_val   = 0.0f;

    for (int oct = 0; oct < CELLULOSE_NOISE_OCTAVES; ++oct)
    {
        value    += Snoise3(x * frequency, y * frequency, z * frequency) * amplitude;
        max_val  += amplitude;
        amplitude *= irregularity;  // each octave weighted by irregularity
        frequency *= 2.0f;
    }
    return (max_val > 0.0f) ? value / max_val : 0.0f;
}

// ---------------------------------------------------------------------------
// Pixel accessor helpers — abstract over 8/16/32-bit formats
// ---------------------------------------------------------------------------

struct PixelF { float r, g, b, a; };

template<typename T>
static inline float ToFloat(T v, float scale);

template<>
inline float ToFloat<A_u_char>(A_u_char v, float scale)
{ return v / scale; }

template<>
inline float ToFloat<A_u_short>(A_u_short v, float scale)
{ return v / scale; }

template<>
inline float ToFloat<PF_FpShort>(PF_FpShort v, float /*scale*/)
{ return v; }

template<typename T>
static inline T FromFloat(float v, float scale);

template<>
inline A_u_char FromFloat<A_u_char>(float v, float scale)
{ return static_cast<A_u_char>(std::clamp(v * scale, 0.0f, scale)); }

template<>
inline A_u_short FromFloat<A_u_short>(float v, float scale)
{ return static_cast<A_u_short>(std::clamp(v * scale, 0.0f, scale)); }

template<>
inline PF_FpShort FromFloat<PF_FpShort>(float v, float /*scale*/)
{ return static_cast<PF_FpShort>(v); }

// Row pointer helpers
template<typename PixelT>
static inline PixelT* RowPtr(PF_EffectWorld* world, int row)
{
    return reinterpret_cast<PixelT*>(
        reinterpret_cast<char*>(world->data) + row * world->rowbytes);
}

// Bilinear sample (float result)
template<typename PixelT>
static PixelF BilinearSample(PF_EffectWorld* world, float u, float v, float scale)
{
    int W = world->width, H = world->height;
    u = std::clamp(u, 0.0f, (float)(W - 1));
    v = std::clamp(v, 0.0f, (float)(H - 1));

    int x0 = (int)u, y0 = (int)v;
    int x1 = std::min(x0 + 1, W - 1);
    int y1 = std::min(y0 + 1, H - 1);
    float tx = u - x0, ty = v - y0;

    PixelT* r0 = RowPtr<PixelT>(world, y0);
    PixelT* r1 = RowPtr<PixelT>(world, y1);

    PixelF p00{ ToFloat(r0[x0].red,   scale), ToFloat(r0[x0].green, scale),
                ToFloat(r0[x0].blue,  scale), ToFloat(r0[x0].alpha, scale) };
    PixelF p10{ ToFloat(r0[x1].red,   scale), ToFloat(r0[x1].green, scale),
                ToFloat(r0[x1].blue,  scale), ToFloat(r0[x1].alpha, scale) };
    PixelF p01{ ToFloat(r1[x0].red,   scale), ToFloat(r1[x0].green, scale),
                ToFloat(r1[x0].blue,  scale), ToFloat(r1[x0].alpha, scale) };
    PixelF p11{ ToFloat(r1[x1].red,   scale), ToFloat(r1[x1].green, scale),
                ToFloat(r1[x1].blue,  scale), ToFloat(r1[x1].alpha, scale) };

    auto lerp = [](float a, float b, float t) { return a + t*(b-a); };

    PixelF out;
    out.r = lerp(lerp(p00.r, p10.r, tx), lerp(p01.r, p11.r, tx), ty);
    out.g = lerp(lerp(p00.g, p10.g, tx), lerp(p01.g, p11.g, tx), ty);
    out.b = lerp(lerp(p00.b, p10.b, tx), lerp(p01.b, p11.b, tx), ty);
    out.a = lerp(lerp(p00.a, p10.a, tx), lerp(p01.a, p11.a, tx), ty);
    return out;
}

// ---------------------------------------------------------------------------
// Core per-pixel render — templated on pixel type + channel scalar type
// ---------------------------------------------------------------------------
template<typename PixelT, typename ChanT>
static void RenderTyped(
    PF_EffectWorld*        input,
    PF_EffectWorld*        output,
    const CelluloseParams& p,
    float                  scale)   // 255 / 32768 / 1.0
{
    int W = input->width, H = input->height;

    // --- Pass 1: compute Sobel edge magnitude for every pixel (parallel over rows) ---
    std::vector<float> edgeMap(W * H, 0.0f);

    std::vector<int> rows(H);
    std::iota(rows.begin(), rows.end(), 0);

    std::for_each(std::execution::par, rows.begin() + 1, rows.end() - 1, [&](int y)
    {
        PixelT* rowM = RowPtr<PixelT>(input, y - 1);
        PixelT* rowC = RowPtr<PixelT>(input, y    );
        PixelT* rowP = RowPtr<PixelT>(input, y + 1);

        for (int x = 1; x < W - 1; ++x)
        {
            auto lum = [scale](const PixelT& px) {
                return 0.299f * ToFloat(px.red,   scale)
                     + 0.587f * ToFloat(px.green, scale)
                     + 0.114f * ToFloat(px.blue,  scale);
            };

            float tl = lum(rowM[x-1]), tc = lum(rowM[x]), tr = lum(rowM[x+1]);
            float ml = lum(rowC[x-1]),                     mr = lum(rowC[x+1]);
            float bl = lum(rowP[x-1]), bc = lum(rowP[x]), br = lum(rowP[x+1]);

            float gx = -tl - 2.0f*ml - bl + tr + 2.0f*mr + br;
            float gy = -tl - 2.0f*tc - tr + bl + 2.0f*bc + br;

            edgeMap[y * W + x] = std::sqrt(gx*gx + gy*gy);
        }
    });

    // Normalise edge map
    float edgeMax = *std::max_element(edgeMap.begin(), edgeMap.end());
    if (edgeMax > 0.0f)
        for (auto& e : edgeMap) e /= edgeMax;

    // Pre-compute noise scale from frame dimensions
    float invW = 1.0f / W;
    float invH = 1.0f / H;

    // Pre-compute constant per-frame values
    const float threshold   = p.edgeSensitivity;
    const int   bleedRadius = (p.colourBleed > 0.0f)
                              ? std::clamp((int)(p.amplitude * 0.1f), 1, 30) : 0;
    const int   blurRadius  = (p.edgeBlur > 0.0f)
                              ? std::max(1, static_cast<int>(p.edgeBlur * 8.0f + 0.5f)) : 0;
    const int   blurR2      = blurRadius * blurRadius;
    const float nt          = p.currentTime * p.frequency;

    // --- Pre-compute noise displacement vectors (parallel over rows) ---
    // Separating this into its own pass gives the scheduler a uniform workload
    // with no nested loops, improving parallel efficiency.
    std::vector<float> noiseX(W * H, 0.0f);
    std::vector<float> noiseY(W * H, 0.0f);

    std::for_each(std::execution::par, rows.begin(), rows.end(), [&](int y)
    {
        for (int x = 0; x < W; ++x)
        {
            if (edgeMap[y * W + x] > threshold)
            {
                int   zoneX  = x / CELLULOSE_ZONE_SIZE;
                int   zoneY  = y / CELLULOSE_ZONE_SIZE;
                float phaseX = (float)(zoneX * 127 + zoneY * 311) * 0.01f;
                float phaseY = (float)(zoneX * 211 + zoneY * 97 ) * 0.01f;
                float nx     = (float)x * invW + phaseX;
                float ny     = (float)y * invH + phaseY;

                noiseX[y * W + x] = LayeredNoise(nx,         ny + 17.3f, nt, p.irregularity);
                noiseY[y * W + x] = LayeredNoise(nx + 31.7f, ny,         nt + 5.1f, p.irregularity);
            }
        }
    });

    // --- Film jitter: smooth Catmull-Rom interpolation between random keypoints ---
    // Keypoints are spaced JITTER_STRIDE frames apart; four surrounding keypoints
    // are sampled and interpolated so the frame-to-frame motion is C1-continuous.
    float jitterX = 0.0f, jitterY = 0.0f;
    if (p.canvasJitter > 0.0f)
    {
        // Hash a (keyframe index, channel seed) pair → [-1, 1]
        auto hashKF = [](int kf, uint32_t seed) -> float {
            uint32_t h = (static_cast<uint32_t>(kf) ^ seed) * 2654435761u;
            h ^= h >> 16;
            h *= 0x45d9f3bu;
            h ^= h >> 16;
            return static_cast<float>(h >> 16) / 32767.5f - 1.0f;
        };

        // Catmull-Rom basis: interpolate p1..p2, t in [0,1], using p0 and p3 as tangent guides
        auto catmullRom = [](float p0, float p1, float p2, float p3, float t) -> float {
            float t2 = t * t, t3 = t2 * t;
            return 0.5f * ((2.0f * p1)
                + (-p0 + p2) * t
                + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2
                + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
        };

        const int JITTER_STRIDE = 6; // keypoint spacing in frames
        int   kf = p.currentFrame / JITTER_STRIDE;
        float t  = static_cast<float>(p.currentFrame % JITTER_STRIDE) / static_cast<float>(JITTER_STRIDE);

        jitterX = catmullRom(hashKF(kf - 1, 0u), hashKF(kf, 0u),
                             hashKF(kf + 1, 0u), hashKF(kf + 2, 0u), t) * p.canvasJitter;
        jitterY = catmullRom(hashKF(kf - 1, 1u), hashKF(kf, 1u),
                             hashKF(kf + 1, 1u), hashKF(kf + 2, 1u), t) * p.canvasJitter * 0.15f;
    }

    // --- Passes 2-5 per pixel (parallel over rows) ---
    std::for_each(std::execution::par, rows.begin(), rows.end(), [&](int y)
    {
        PixelT* outRow = RowPtr<PixelT>(output, y);

        for (int x = 0; x < W; ++x)
        {
            float edge = edgeMap[y * W + x];

            float srcU = static_cast<float>(x) + jitterX;
            float srcV = static_cast<float>(y) + jitterY;

            if (edge > threshold)
            {
                // Passes 2 & 3: read pre-computed noise displacement vectors
                float ndx = noiseX[y * W + x];
                float ndy = noiseY[y * W + x];

                float strength = (edge - threshold) / (1.0f - threshold + 1e-6f);
                float disp = p.amplitude * strength;

                srcU = std::clamp(srcU + ndx * disp, 0.0f, (float)(W - 1));
                srcV = std::clamp(srcV + ndy * disp, 0.0f, (float)(H - 1));
            }

            // Bilinear sample displaced position
            PixelF displaced = BilinearSample<PixelT>(input, srcU, srcV, scale);

            // Pass 4: colour bleed — weighted average with edge neighbours
            if (edge > threshold && p.colourBleed > 0.0f)
            {
                float sumR=0, sumG=0, sumB=0, sumA=0, weight=0;

                for (int bdy = -bleedRadius; bdy <= bleedRadius; ++bdy)
                {
                    int sy = std::clamp(y + bdy, 0, H - 1);
                    PixelT* nr = RowPtr<PixelT>(input, sy);   // hoisted out of inner loop
                    const float* edgeRow = edgeMap.data() + sy * W;

                    for (int bdx = -bleedRadius; bdx <= bleedRadius; ++bdx)
                    {
                        int sx = std::clamp(x + bdx, 0, W - 1);
                        if (edgeRow[sx] > threshold)
                        {
                            float w = 1.0f / (1.0f + std::sqrt((float)(bdx*bdx + bdy*bdy)));
                            sumR += ToFloat(nr[sx].red,   scale) * w;
                            sumG += ToFloat(nr[sx].green, scale) * w;
                            sumB += ToFloat(nr[sx].blue,  scale) * w;
                            sumA += ToFloat(nr[sx].alpha, scale) * w;
                            weight += w;
                        }
                    }
                }

                if (weight > 0.0f)
                {
                    float bleed = p.colourBleed;
                    displaced.r = displaced.r * (1-bleed) + (sumR/weight) * bleed;
                    displaced.g = displaced.g * (1-bleed) + (sumG/weight) * bleed;
                    displaced.b = displaced.b * (1-bleed) + (sumB/weight) * bleed;
                    displaced.a = displaced.a * (1-bleed) + (sumA/weight) * bleed;
                }
            }

            // Pass 5: Edge blur — circular defocus along object boundaries
            if (edge > threshold && p.edgeBlur > 0.0f)
            {
                float strength = (edge - threshold) / (1.0f - threshold + 1e-6f);
                float sumR=0, sumG=0, sumB=0, sumA=0, totalW=0;

                for (int ky = -blurRadius; ky <= blurRadius; ++ky)
                {
                    for (int kx = -blurRadius; kx <= blurRadius; ++kx)
                    {
                        if (kx*kx + ky*ky > blurR2) continue;
                        PixelF s = BilinearSample<PixelT>(input,
                            std::clamp(srcU + kx, 0.0f, static_cast<float>(W - 1)),
                            std::clamp(srcV + ky, 0.0f, static_cast<float>(H - 1)),
                            scale);
                        sumR += s.r; sumG += s.g;
                        sumB += s.b; sumA += s.a;
                        totalW += 1.0f;
                    }
                }

                if (totalW > 0.0f)
                {
                    float blend = strength * p.edgeBlur;
                    displaced.r = displaced.r * (1-blend) + (sumR/totalW) * blend;
                    displaced.g = displaced.g * (1-blend) + (sumG/totalW) * blend;
                    displaced.b = displaced.b * (1-blend) + (sumB/totalW) * blend;
                    displaced.a = displaced.a * (1-blend) + (sumA/totalW) * blend;
                }
            }

            // Write output
            outRow[x].red   = FromFloat<ChanT>(displaced.r, scale);
            outRow[x].green = FromFloat<ChanT>(displaced.g, scale);
            outRow[x].blue  = FromFloat<ChanT>(displaced.b, scale);
            outRow[x].alpha = FromFloat<ChanT>(displaced.a, scale);
        }
    });
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// PF_Pixel channel typedef helpers for FromFloat template dispatch
// ---------------------------------------------------------------------------
// AE's pixel structs don't expose a ::Channel typedef, so we specialise via
// free helper structs instead.

template<typename PixelT> struct PixelChannel {};
template<> struct PixelChannel<PF_Pixel8>     { using type = A_u_char;  static constexpr float scale = 255.0f; };
template<> struct PixelChannel<PF_Pixel16>    { using type = A_u_short; static constexpr float scale = 32768.0f; };
template<> struct PixelChannel<PF_PixelFloat> { using type = PF_FpShort;static constexpr float scale = 1.0f; };

// Re-implement RenderTyped using the helper struct
namespace
{

template<typename PixelT>
static void RenderTypedEx(
    PF_EffectWorld*        input,
    PF_EffectWorld*        output,
    const CelluloseParams& p)
{
    RenderTyped<PixelT, typename PixelChannel<PixelT>::type>(input, output, p, PixelChannel<PixelT>::scale);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------
PF_Err CelluloseCPU::Render(
    PF_InData*             /*in_data*/,
    PF_EffectWorld*        input,
    PF_EffectWorld*        output,
    const CelluloseParams& params)
{
    switch (params.bitDepth)
    {
    case 16:
        RenderTypedEx<PF_Pixel16>(input, output, params);
        break;
    case 32:
        RenderTypedEx<PF_PixelFloat>(input, output, params);
        break;
    case 8:
    default:
        RenderTypedEx<PF_Pixel8>(input, output, params);
        break;
    }
    return PF_Err_NONE;
}
