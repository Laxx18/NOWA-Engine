// ============================================================================
// UNDERWATER POST-PROCESS SHADER WITH DEPTH TEXTURE (Metal)
// Uses Ogre's auto-bound projection parameters
// ============================================================================

#include <metal_stdlib>
using namespace metal;

struct PS_INPUT
{
    float4 position [[position]];
    float2 uv0;
};

struct Uniforms
{
    // Core
    float underwaterFactor;
    float timeSeconds;

    // Colors
    float3 waterTint;
    float3 deepWaterTint;

    // Fog
    float fogDensity;
    float maxFogDepth;

    // Effects
    float absorptionScale;
    float godRayStrength;
    float causticStrength;
    float maxCausticDepth;
    float particleStrength;
    float maxParticleDepth;
    float bubbleStrength;

    // Visual
    float distortion;
    float contrast;
    float saturation;
    float vignette;
    float chromaticAberration;

    // Sun
    float2 sunScreenPos;

    // Depth reconstruction params (same as SSAO)
    float2 projectionParams;
    float farClipDistance;
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

float3 applySaturation(float3 c, float s)
{
    float l = dot(c, float3(0.2126, 0.7152, 0.0722));
    return mix(float3(l), c, s);
}

float hash(float2 p)
{
    return fract(sin(dot(p, float2(127.1, 311.7))) * 43758.5453);
}

float noise(float2 p)
{
    float2 i = floor(p);
    float2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + float2(1.0, 0.0));
    float c = hash(i + float2(0.0, 1.0));
    float d = hash(i + float2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(float2 p, int octaves)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < octaves; i++)
    {
        value += amplitude * noise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }

    return value;
}

// ============================================================================
// DEPTH LINEARIZATION - Same formula as SSAO
// ============================================================================
float getLinearDepth(float rawDepth, float2 projParams, float farClip)
{
    // Avoid division by zero
    float denom = rawDepth - projParams.x;
    if (abs(denom) < 0.0001)
        denom = 0.0001;

    // This gives normalized depth [0, 1] because projectionParams.y was divided by farClip
    float normalizedDepth = projParams.y / denom;

    // Convert back to world units
    return normalizedDepth * farClip;
}

// ============================================================================
// UNDERWATER FOG - Depth-based fog with color gradient
// ============================================================================
float3 calculateUnderwaterFog(
    float3 sceneColor,
    float linearDepth,
    float3 waterColor,
    float3 deepWaterColor,
    float density,
    float maxDepth
)
{
    // Normalize depth for color blending
    float normalizedDepth = saturate(linearDepth / maxDepth);

    // Exponential fog - more realistic
    float fogFactor = 1.0 - exp(-density * linearDepth);
    fogFactor = saturate(fogFactor);

    // Depth-based fog color transition
    float3 fogColor = mix(waterColor, deepWaterColor, pow(normalizedDepth, 0.8));

    return mix(sceneColor, fogColor, fogFactor);
}

// ============================================================================
// LIGHT ABSORPTION - Red absorbs first, blue travels furthest
// ============================================================================
float3 applyLightAbsorption(float3 color, float linearDepth, float absScale)
{
    // Absorption coefficients: red > green > blue
    float3 absorption = float3(0.4, 0.15, 0.05) * absScale;
    float3 transmitted = exp(-absorption * linearDepth);
    // Keep minimum color to avoid total blackout
    transmitted = max(transmitted, float3(0.1, 0.2, 0.4));
    return color * transmitted;
}

// ============================================================================
// CAUSTICS - Light patterns on surfaces (only on geometry, not in water volume)
// ============================================================================
float calculateCaustics(float2 uv, float linearDepth, float time, float maxDepth)
{
    // Only show caustics on nearby surfaces
    float depthFactor = 1.0 - saturate(linearDepth / maxDepth);
    depthFactor = pow(depthFactor, 2.5);  // Sharper falloff

    if (depthFactor < 0.01)
        return 0.0;

    // Use world-space-like UVs based on depth for better projection
    float2 worldUV = uv * (10.0 + linearDepth * 0.5);

    // Multiple overlapping caustic patterns
    float2 uv1 = worldUV + float2(time * 0.02, time * 0.015);
    float2 uv2 = worldUV * 1.3 - float2(time * 0.018, -time * 0.022);

    // Create caustic pattern using noise
    float c1 = noise(uv1 * 3.0);
    float c2 = noise(uv2 * 2.5);

    // Combine and sharpen
    float caustic = (c1 + c2) * 0.5;
    caustic = pow(caustic, 3.0) * 4.0;  // Sharpen the caustic pattern

    return caustic * depthFactor;
}

// ============================================================================
// GOD RAYS - Volumetric light shafts from surface
// ============================================================================
float3 calculateGodRays(float2 uv, float linearDepth, float2 sunPos, float time, float maxRayDepth)
{
    // Fade out with depth
    float visibility = 1.0 - saturate(linearDepth / maxRayDepth);
    visibility = pow(visibility, 0.5);

    if (visibility < 0.01)
        return float3(0.0);

    float2 toSun = sunPos - uv;
    float sunDist = length(toSun);
    float2 sunDir = toSun / max(sunDist, 0.001);

    float rayAccum = 0.0;
    const int NUM_SAMPLES = 16;
    float stepSize = min(sunDist, 0.5) / float(NUM_SAMPLES);

    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        float2 samplePos = uv + sunDir * stepSize * float(i);
        float rayNoise = fbm(samplePos * 3.0 + float2(time * 0.03, time * 0.02), 3);
        float radialFalloff = 1.0 - saturate(float(i) / float(NUM_SAMPLES));
        rayAccum += rayNoise * radialFalloff * radialFalloff;
    }

    rayAccum = (rayAccum / float(NUM_SAMPLES)) * visibility;

    // Sun glow
    float sunGlow = pow(saturate(1.0 - sunDist * 1.5), 4.0) * 0.4 * visibility;
    rayAccum += sunGlow;

    return float3(0.4, 0.7, 0.9) * rayAccum;
}

// ============================================================================
// PARTICLES - Floating debris/plankton
// ============================================================================
float getParticles(float2 uv, float linearDepth, float time, float maxDepth)
{
    float visibility = 1.0 - saturate(linearDepth / maxDepth);
    visibility = pow(visibility, 0.7);

    if (visibility < 0.01)
        return 0.0;

    float particles = 0.0;

    for (int p = 0; p < 25; p++)
    {
        float seed = float(p);
        float pDepth = hash(float2(seed, seed + 10.0));

        float speedMult = 0.3 + pDepth * 0.4;
        float2 pPos = float2(
            fract(hash(float2(seed * 43.0, seed)) + time * 0.02 * speedMult),
            fract(hash(float2(seed * 37.0, seed + 5.0)) + time * 0.03 * speedMult)
        );

        // Gentle swaying motion
        pPos.x += sin(time * 1.5 + seed * 2.0) * 0.015;
        pPos.y += cos(time * 1.2 + seed * 1.5) * 0.01;

        float size = 0.002 + hash(float2(seed + 5.0, seed)) * 0.004;
        float dist = length(uv - pPos);
        float particle = smoothstep(size, 0.0, dist);

        particles += particle * (0.3 + pDepth * 0.5);
    }

    return particles * visibility;
}

// ============================================================================
// BUBBLES - Rising from bottom of screen
// ============================================================================
float getBubbles(float2 uv, float time)
{
    float bubbles = 0.0;

    // Flip Y so bubbles rise from bottom
    float2 bubbleUV = float2(uv.x, 1.0 - uv.y);

    for (int b = 0; b < 12; b++)
    {
        float seed = float(b);
        float speed = 0.08 + hash(float2(seed, seed + 1.0)) * 0.12;

        // Y position cycles from 0 to 1 (bottom to top in flipped coords)
        float yOffset = fract(time * speed + hash(float2(seed * 7.0, seed)));

        // Only show in lower 70% of screen (upper 70% in original coords)
        if (yOffset > 0.7) continue;

        float2 bPos = float2(
            hash(float2(seed * 23.0, seed + 3.0)),
            yOffset
        );

        // Wobble as they rise
        bPos.x += sin(time * 2.5 + seed * 4.0) * 0.025 * (1.0 - yOffset);

        float size = 0.004 + hash(float2(seed + 2.0, seed)) * 0.006;
        float dist = length(bubbleUV - bPos);

        // Bubble with highlight
        float bubble = smoothstep(size, size * 0.4, dist);
        float highlight = smoothstep(size * 0.5, 0.0, length(bubbleUV - bPos + float2(size * 0.3, -size * 0.3)));

        bubbles += bubble * 0.5 + highlight * 0.8;
    }

    return saturate(bubbles);
}

// ============================================================================
// MAIN SHADER
// ============================================================================
fragment float4 main_metal(
    PS_INPUT inPs [[stage_in]],
    texture2d<float> rt0 [[texture(0)]],
    texture2d<float> depthTexture [[texture(1)]],
    sampler samplerBilinear [[sampler(0)]],
    sampler samplerPoint [[sampler(1)]],
    constant Uniforms& uniforms [[buffer(0)]]
)
{
    float2 uv = inPs.uv0;

    // ========================================================================
    // GET LINEAR DEPTH
    // ========================================================================
    float rawDepth = depthTexture.sample(samplerPoint, uv).x;
    float linearDepth = getLinearDepth(rawDepth, uniforms.projectionParams, uniforms.farClipDistance);

    // Clamp to reasonable range
    linearDepth = clamp(linearDepth, 0.1, uniforms.farClipDistance);

    // ========================================================================
    // WAVE DISTORTION
    // ========================================================================
    // Stronger distortion, reduced slightly at distance
    float distortionFactor = 1.0 - saturate(linearDepth / uniforms.maxFogDepth) * 0.3;

    float wave1 = sin(uv.x * 15.0 + uniforms.timeSeconds * 0.8) * cos(uv.y * 12.0 - uniforms.timeSeconds * 0.6);
    float wave2 = cos(uv.x * 20.0 - uniforms.timeSeconds * 0.7) * sin(uv.y * 18.0 + uniforms.timeSeconds * 0.75);
    float wave3 = sin((uv.x + uv.y) * 25.0 + uniforms.timeSeconds * 1.2);

    float2 distortionVec = float2(
        wave1 * 0.5 + wave2 * 0.3 + wave3 * 0.2,
        wave2 * 0.5 + wave1 * 0.3 + wave3 * 0.2
    ) * uniforms.distortion * uniforms.underwaterFactor * distortionFactor;

    // ========================================================================
    // CHROMATIC ABERRATION
    // ========================================================================
    float2 centerOffset = uv - 0.5;
    float aberrationAmount = uniforms.chromaticAberration * length(centerOffset) * 2.0 * uniforms.underwaterFactor;
    float2 aberrationDir = normalize(centerOffset + 0.001);

    float2 uvR = uv + distortionVec + aberrationDir * aberrationAmount;
    float2 uvG = uv + distortionVec;
    float2 uvB = uv + distortionVec - aberrationDir * aberrationAmount;

    float r = rt0.sample(samplerBilinear, uvR).r;
    float g = rt0.sample(samplerBilinear, uvG).g;
    float b = rt0.sample(samplerBilinear, uvB).b;
    float3 col = float3(r, g, b);

    // ========================================================================
    // LIGHT ABSORPTION (apply before fog)
    // ========================================================================
    col = applyLightAbsorption(col, linearDepth, uniforms.absorptionScale * uniforms.underwaterFactor);

    // ========================================================================
    // UNDERWATER FOG
    // ========================================================================
    col = calculateUnderwaterFog(
        col,
        linearDepth,
        uniforms.waterTint,
        uniforms.deepWaterTint,
        uniforms.fogDensity * uniforms.underwaterFactor,
        uniforms.maxFogDepth
    );

    // ========================================================================
    // CAUSTICS (only on nearby surfaces)
    // ========================================================================
    float caustics = calculateCaustics(uv, linearDepth, uniforms.timeSeconds, uniforms.maxCausticDepth);
    col += caustics * uniforms.causticStrength * uniforms.underwaterFactor * float3(0.5, 0.7, 0.9);

    // ========================================================================
    // GOD RAYS
    // ========================================================================
    float3 godRays = calculateGodRays(uv, linearDepth, uniforms.sunScreenPos, uniforms.timeSeconds, uniforms.maxFogDepth * 0.5);
    col += godRays * uniforms.godRayStrength * uniforms.underwaterFactor;

    // ========================================================================
    // PARTICLES
    // ========================================================================
    float particles = getParticles(uv, linearDepth, uniforms.timeSeconds, uniforms.maxParticleDepth);
    col += particles * uniforms.particleStrength * uniforms.underwaterFactor * float3(0.6, 0.75, 0.85);

    // ========================================================================
    // BUBBLES
    // ========================================================================
    float bubbles = getBubbles(uv, uniforms.timeSeconds);
    col += bubbles * uniforms.bubbleStrength * uniforms.underwaterFactor * float3(0.8, 0.9, 1.0);

    // ========================================================================
    // COLOR GRADING - Shift towards blue/green
    // ========================================================================
    col.r *= mix(1.0, 0.7, uniforms.underwaterFactor);
    col.g *= mix(1.0, 0.9, uniforms.underwaterFactor);
    col.b *= mix(1.0, 1.1, uniforms.underwaterFactor);

    // ========================================================================
    // CONTRAST
    // ========================================================================
    float3 midGray = float3(0.5);
    col = mix(midGray, col, mix(1.0, uniforms.contrast, uniforms.underwaterFactor));

    // ========================================================================
    // SATURATION
    // ========================================================================
    col = applySaturation(col, mix(1.0, uniforms.saturation, uniforms.underwaterFactor));

    // ========================================================================
    // VIGNETTE
    // ========================================================================
    float2 vignetteUV = (uv - 0.5) * 2.0;
    float vignetteDist = length(vignetteUV);
    float vignetteAmount = 1.0 - pow(saturate(vignetteDist * 0.7), uniforms.vignette);
    col *= mix(1.0, vignetteAmount, uniforms.underwaterFactor * 0.6);

    return float4(saturate(col), 1.0);
}
