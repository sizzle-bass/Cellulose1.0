// ---------------------------------------------------------------------------
// CelluloseMetal.metal
// Metal compute shader — 4-pass celluloid colour-boundary oscillation.
//
// Same algorithm as CelluloseCUDA.cu, rewritten in Metal Shading Language.
// Compiled offline to CelluloseMetal.metallib by the CMake build.
// ---------------------------------------------------------------------------

#include <metal_stdlib>
using namespace metal;

// ---------------------------------------------------------------------------
// Uniforms (must match MetalUniforms struct in CelluloseMetal.mm)
// ---------------------------------------------------------------------------
struct Uniforms
{
    uint  width;
    uint  height;
    int   currentFrame;
    float amplitude;
    float frequency;
    float irregularity;
    float edgeSensitivity;
    float colourBleed;
    float edgeBlur;
    float canvasJitter;
    float currentTime;
    float crushBlacks;
    float vibrance;
    float vibranceFocus;
};

// ---------------------------------------------------------------------------
// Constants matching CelluloseAlgorithm.h
// ---------------------------------------------------------------------------
constant int   NOISE_OCTAVES = 3;
constant int   ZONE_SIZE     = 32;
constant float F3            = 0.333333333f;
constant float G3            = 0.166666667f;

// ---------------------------------------------------------------------------
// 3D Simplex noise — Stefan Gustavson, ported to MSL
// ---------------------------------------------------------------------------
constant int perm[512] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
    60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
    200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
    52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
    207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
    119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
    129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
    218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
    184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
    222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
    60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
    200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
    52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
    207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
    119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
    129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
    218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
    184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
    222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

constant float3 grad3[12] = {
    {1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},
    {1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},
    {0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1}
};

static float snoise3(float xin, float yin, float zin)
{
    float s  = (xin + yin + zin) * F3;
    int i = (int)floor(xin + s);
    int j = (int)floor(yin + s);
    int k = (int)floor(zin + s);

    float t  = (float)(i + j + k) * G3;
    float x0 = xin - (i - t);
    float y0 = yin - (j - t);
    float z0 = zin - (k - t);

    int i1, j1, k1, i2, j2, k2;
    if (x0 >= y0) {
        if      (y0 >= z0) { i1=1;j1=0;k1=0; i2=1;j2=1;k2=0; }
        else if (x0 >= z0) { i1=1;j1=0;k1=0; i2=1;j2=0;k2=1; }
        else               { i1=0;j1=0;k1=1; i2=1;j2=0;k2=1; }
    } else {
        if      (y0 < z0)  { i1=0;j1=0;k1=1; i2=0;j2=1;k2=1; }
        else if (x0 < z0)  { i1=0;j1=1;k1=0; i2=0;j2=1;k2=1; }
        else               { i1=0;j1=1;k1=0; i2=1;j2=1;k2=0; }
    }

    float x1=x0-i1+G3, y1=y0-j1+G3, z1=z0-k1+G3;
    float x2=x0-i2+2.0f*G3, y2=y0-j2+2.0f*G3, z2=z0-k2+2.0f*G3;
    float x3=x0-1.0f+3.0f*G3, y3=y0-1.0f+3.0f*G3, z3=z0-1.0f+3.0f*G3;

    int ii=i&255, jj=j&255, kk=k&255;
    int gi0 = perm[ii+   perm[jj+   perm[kk   ]]] % 12;
    int gi1 = perm[ii+i1+perm[jj+j1+perm[kk+k1]]] % 12;
    int gi2 = perm[ii+i2+perm[jj+j2+perm[kk+k2]]] % 12;
    int gi3 = perm[ii+1 +perm[jj+1 +perm[kk+1 ]]] % 12;

    float n0=0, n1=0, n2=0, n3=0;
    float t0 = 0.6f - x0*x0 - y0*y0 - z0*z0;
    if (t0 > 0) { t0*=t0; n0 = t0*t0 * dot(grad3[gi0], float3(x0,y0,z0)); }
    float t1 = 0.6f - x1*x1 - y1*y1 - z1*z1;
    if (t1 > 0) { t1*=t1; n1 = t1*t1 * dot(grad3[gi1], float3(x1,y1,z1)); }
    float t2 = 0.6f - x2*x2 - y2*y2 - z2*z2;
    if (t2 > 0) { t2*=t2; n2 = t2*t2 * dot(grad3[gi2], float3(x2,y2,z2)); }
    float t3 = 0.6f - x3*x3 - y3*y3 - z3*z3;
    if (t3 > 0) { t3*=t3; n3 = t3*t3 * dot(grad3[gi3], float3(x3,y3,z3)); }

    return 32.0f * (n0 + n1 + n2 + n3);
}

static float layeredNoise(float x, float y, float z, float irregularity)
{
    float value=0, amplitude=1, frequency=1, maxVal=0;
    for (int oct=0; oct<NOISE_OCTAVES; ++oct)
    {
        value    += snoise3(x*frequency, y*frequency, z*frequency) * amplitude;
        maxVal   += amplitude;
        amplitude *= irregularity;
        frequency *= 2.0f;
    }
    return (maxVal > 0.0f) ? value/maxVal : 0.0f;
}

// ---------------------------------------------------------------------------
// Bilinear sample from flat float4 buffer
// ---------------------------------------------------------------------------
static float4 bilinear(const device float4* src, uint W, uint H, float u, float v)
{
    u = clamp(u, 0.0f, (float)(W-1));
    v = clamp(v, 0.0f, (float)(H-1));
    uint x0=(uint)u, y0=(uint)v;
    uint x1=min(x0+1u, W-1u), y1=min(y0+1u, H-1u);
    float tx=u-x0, ty=v-y0;

    float4 p00=src[y0*W+x0], p10=src[y0*W+x1];
    float4 p01=src[y1*W+x0], p11=src[y1*W+x1];

    float4 lo = mix(p00, p10, tx);
    float4 hi = mix(p01, p11, tx);
    return mix(lo, hi, ty);
}

// ---------------------------------------------------------------------------
// Threadgroup shared memory tile (16+2 halo)
// ---------------------------------------------------------------------------
#define TG_W 18
#define TG_H 18

// ---------------------------------------------------------------------------
// Main compute kernel
// ---------------------------------------------------------------------------
kernel void cellulose_main(
    const device   float4* src          [[buffer(0)]],
          device   float4* dst          [[buffer(1)]],
    const device   Uniforms& u          [[buffer(2)]],
    threadgroup    float     tile[TG_H][TG_W] [[threadgroup(0)]],
    uint2 gid   [[thread_position_in_grid]],
    uint2 lid   [[thread_position_in_threadgroup]],
    uint2 tgSize[[threads_per_threadgroup]])
{
    uint W = u.width, H = u.height;

    // ---------- Pass 1: Sobel via threadgroup shared tile ----------
    uint lx = lid.x + 1, ly = lid.y + 1;

    auto lum = [](float4 p) { return 0.299f*p.x + 0.587f*p.y + 0.114f*p.z; };
    auto clampLoad = [&](int cx, int cy) -> float {
        cx = clamp(cx, 0, (int)(W-1));
        cy = clamp(cy, 0, (int)(H-1));
        return lum(src[(uint)cy*W + (uint)cx]);
    };

    if (gid.x < W && gid.y < H)
        tile[ly][lx] = lum(src[gid.y*W + gid.x]);

    if (lid.x == 0)             tile[ly][0]      = clampLoad((int)gid.x-1, (int)gid.y);
    if (lid.x == tgSize.x-1)   tile[ly][TG_W-1] = clampLoad((int)gid.x+1, (int)gid.y);
    if (lid.y == 0)             tile[0][lx]      = clampLoad((int)gid.x,   (int)gid.y-1);
    if (lid.y == tgSize.y-1)   tile[TG_H-1][lx] = clampLoad((int)gid.x,   (int)gid.y+1);

    if (lid.x==0 && lid.y==0)
        tile[0][0] = clampLoad((int)gid.x-1,(int)gid.y-1);
    if (lid.x==tgSize.x-1 && lid.y==0)
        tile[0][TG_W-1] = clampLoad((int)gid.x+1,(int)gid.y-1);
    if (lid.x==0 && lid.y==tgSize.y-1)
        tile[TG_H-1][0] = clampLoad((int)gid.x-1,(int)gid.y+1);
    if (lid.x==tgSize.x-1 && lid.y==tgSize.y-1)
        tile[TG_H-1][TG_W-1] = clampLoad((int)gid.x+1,(int)gid.y+1);

    threadgroup_barrier(mem_flags::mem_threadgroup);

    if (gid.x >= W || gid.y >= H) return;

    float tl=tile[ly-1][lx-1], tc=tile[ly-1][lx], tr=tile[ly-1][lx+1];
    float ml=tile[ly  ][lx-1],                      mr=tile[ly  ][lx+1];
    float bl=tile[ly+1][lx-1], bc=tile[ly+1][lx], br=tile[ly+1][lx+1];

    float gx_s = -tl - 2.0f*ml - bl + tr + 2.0f*mr + br;
    float gy_s = -tl - 2.0f*tc - tr + bl + 2.0f*bc + br;
    float edge = min(sqrt(gx_s*gx_s + gy_s*gy_s) * 0.25f, 1.0f);

    // Canvas jitter: deterministic per-frame whole-gate displacement
    float jitterX = 0.0f, jitterY = 0.0f;
    if (u.canvasJitter > 0.0f)
    {
        uint h = (uint)u.currentFrame * 2654435761u;
        h ^= h >> 16; h *= 0x45d9f3bu; h ^= h >> 16;
        jitterX = ((float)(h >> 16) / 32767.5f - 1.0f) * u.canvasJitter;
        h *= 2246822519u; h ^= h >> 13;
        jitterY = ((float)(h >> 16) / 32767.5f - 1.0f) * u.canvasJitter * 0.15f;
    }


    float srcU = (float)gid.x + jitterX;
    float srcV = (float)gid.y + jitterY;

    // ---------- Passes 2 & 3: noise + displacement ----------
    if (edge > u.edgeSensitivity)
    {
        float invW = 1.0f / W, invH = 1.0f / H;
        int zoneX = (int)gid.x / ZONE_SIZE, zoneY = (int)gid.y / ZONE_SIZE;
        float phaseX = (float)(zoneX*127 + zoneY*311) * 0.01f;
        float phaseY = (float)(zoneX*211 + zoneY*97 ) * 0.01f;

        float nx = (float)gid.x * invW + phaseX;
        float ny = (float)gid.y * invH + phaseY;
        float nt = u.currentTime * u.frequency;

        float dx = layeredNoise(nx,        ny+17.3f, nt,       u.irregularity);
        float dy = layeredNoise(nx+31.7f,  ny,       nt+5.1f,  u.irregularity);

        float strength = (edge - u.edgeSensitivity) / (1.0f - u.edgeSensitivity + 1e-6f);
        float disp = u.amplitude * strength;

        srcU = clamp(srcU + dx*disp, 0.0f, (float)(W-1));
        srcV = clamp(srcV + dy*disp, 0.0f, (float)(H-1));
    }

    float4 result = bilinear(src, W, H, srcU, srcV);

    // ---------- Pass 4: colour bleed ----------
    if (edge > u.edgeSensitivity && u.colourBleed > 0.0f)
    {
        int radius = max(1, (int)(u.amplitude * 0.5f));
        radius = min(radius, 4);

        float sumR=0,sumG=0,sumB=0,sumA=0,weight=0;
        for (int dy=-radius; dy<=radius; ++dy)
        {
            int sy = clamp((int)gid.y+dy, 0, (int)(H-1));
            for (int dx2=-radius; dx2<=radius; ++dx2)
            {
                int sx = clamp((int)gid.x+dx2, 0, (int)(W-1));
                float4 n = src[(uint)sy*W + (uint)sx];
                float w = 1.0f / (1.0f + sqrt((float)(dx2*dx2 + dy*dy)));
                sumR += n.x*w; sumG += n.y*w; sumB += n.z*w; sumA += n.w*w;
                weight += w;
            }
        }
        if (weight > 0.0f)
        {
            result.x = result.x*(1-u.colourBleed) + (sumR/weight)*u.colourBleed;
            result.y = result.y*(1-u.colourBleed) + (sumG/weight)*u.colourBleed;
            result.z = result.z*(1-u.colourBleed) + (sumB/weight)*u.colourBleed;
            result.w = result.w*(1-u.colourBleed) + (sumA/weight)*u.colourBleed;
        }
    }

    // ---------- Pass 5: edge blur ----------
    if (edge > u.edgeSensitivity && u.edgeBlur > 0.0f)
    {
        float strength = (edge - u.edgeSensitivity) / (1.0f - u.edgeSensitivity + 1e-6f);
        int   radius   = max(1, (int)(u.edgeBlur * 8.0f + 0.5f));
        int   r2       = radius * radius;

        float sumR=0,sumG=0,sumB=0,sumA=0,totalW=0;
        for (int ky = -radius; ky <= radius; ++ky)
        {
            for (int kx = -radius; kx <= radius; ++kx)
            {
                if (kx*kx + ky*ky > r2) continue;
                float4 s = bilinear(src, W, H,
                    clamp(srcU + kx, 0.0f, (float)(W-1)),
                    clamp(srcV + ky, 0.0f, (float)(H-1)));
                sumR += s.x; sumG += s.y; sumB += s.z; sumA += s.w;
                totalW += 1.0f;
            }
        }
        if (totalW > 0.0f)
        {
            float blend = strength * u.edgeBlur;
            result.x = result.x*(1-blend) + (sumR/totalW)*blend;
            result.y = result.y*(1-blend) + (sumG/totalW)*blend;
            result.z = result.z*(1-blend) + (sumB/totalW)*blend;
            result.w = result.w*(1-blend) + (sumA/totalW)*blend;
        }
    }

    // Post-process: crush blacks
    if (u.crushBlacks > 0.0f)
    {
        float lum = 0.299f*result.x + 0.587f*result.y + 0.114f*result.z;
        float shadowWeight = (1.0f - lum) * (1.0f - lum);
        float crush = max(0.0f, 1.0f - u.crushBlacks * shadowWeight);
        result.x *= crush;
        result.y *= crush;
        result.z *= crush;
    }

    // Post-process: vibrance — boost saturation to counter milky cast
    if (u.vibrance > 0.0f)
    {
        float lum    = 0.299f*result.x + 0.587f*result.y + 0.114f*result.z;
        float chroma = max(max(result.x, result.y), result.z)
                     - min(min(result.x, result.y), result.z);
        float boost  = 1.0f - chroma * u.vibranceFocus;
        float sat    = 1.0f + u.vibrance * boost;
        result.x = clamp(lum + (result.x - lum) * sat, 0.0f, 1.0f);
        result.y = clamp(lum + (result.y - lum) * sat, 0.0f, 1.0f);
        result.z = clamp(lum + (result.z - lum) * sat, 0.0f, 1.0f);
    }

    dst[gid.y*W + gid.x] = result;
}
