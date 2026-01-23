// ============================================================================
// UNDERWATER POST-PROCESS SHADER WITH DEPTH TEXTURE
// Uses Ogre's auto-bound projection parameters
// ============================================================================

Texture2D<float4> rt0          : register(t0);  // Scene color
Texture2D<float>  depthTexture : register(t1);  // Depth buffer

SamplerState samplerBilinear   : register(s0);  // For color
SamplerState samplerPoint      : register(s1);  // For depth

struct PS_INPUT
{
    float2 uv0 : TEXCOORD0;
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static float3 applySaturation(float3 c, float s)
{
    float l = dot(c, float3(0.2126f, 0.7152f, 0.0722f));
    return lerp(l.xxx, c, s);
}

static float hash(float2 p)
{
    return frac(sin(dot(p, float2(127.1f, 311.7f))) * 43758.5453f);
}

static float noise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    f = f * f * (3.0f - 2.0f * f);
    
    float a = hash(i);
    float b = hash(i + float2(1.0f, 0.0f));
    float c = hash(i + float2(0.0f, 1.0f));
    float d = hash(i + float2(1.0f, 1.0f));
    
    return lerp(lerp(a, b, f.x), lerp(c, d, f.x), f.y);
}

static float fbm(float2 p, int octaves)
{
    float value = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    
    for (int i = 0; i < octaves; i++)
    {
        value += amplitude * noise(p * frequency);
        frequency *= 2.0f;
        amplitude *= 0.5f;
    }
    
    return value;
}

// ============================================================================
// DEPTH LINEARIZATION - Same formula as SSAO
// ============================================================================
static float getLinearDepth(float rawDepth, float2 projectionParams, float farClipDistance)
{
    // Avoid division by zero
    float denom = rawDepth - projectionParams.x;
    if (abs(denom) < 0.0001f) 
        denom = 0.0001f;
    
    // This gives normalized depth [0, 1] because projectionParams.y was divided by farClip
    float normalizedDepth = projectionParams.y / denom;
    
    // Convert back to world units
    return normalizedDepth * farClipDistance;
}

// ============================================================================
// UNDERWATER FOG - Depth-based fog with color gradient
// ============================================================================
static float3 calculateUnderwaterFog(
    float3 sceneColor,
    float linearDepth,
    float3 waterColor,
    float3 deepWaterColor,
    float fogDensity,
    float maxFogDepth
)
{
    // Normalize depth for color blending
    float normalizedDepth = saturate(linearDepth / maxFogDepth);
    
    // Exponential fog - more realistic
    float fogFactor = 1.0f - exp(-fogDensity * linearDepth);
    fogFactor = saturate(fogFactor);
    
    // Depth-based fog color transition
    float3 fogColor = lerp(waterColor, deepWaterColor, pow(normalizedDepth, 0.8f));
    
    return lerp(sceneColor, fogColor, fogFactor);
}

// ============================================================================
// LIGHT ABSORPTION - Red absorbs first, blue travels furthest
// ============================================================================
static float3 applyLightAbsorption(float3 color, float linearDepth, float absorptionScale)
{
    // Absorption coefficients: red > green > blue
    float3 absorption = float3(0.4f, 0.15f, 0.05f) * absorptionScale;
    float3 transmitted = exp(-absorption * linearDepth);
    // Keep minimum color to avoid total blackout
    transmitted = max(transmitted, float3(0.1f, 0.2f, 0.4f));
    return color * transmitted;
}

// ============================================================================
// CAUSTICS - Light patterns on surfaces (only on geometry, not in water volume)
// ============================================================================
static float calculateCaustics(float2 uv, float linearDepth, float time, float maxCausticDepth)
{
    // Only show caustics on nearby surfaces
    float depthFactor = 1.0f - saturate(linearDepth / maxCausticDepth);
    depthFactor = pow(depthFactor, 2.5f);  // Sharper falloff
    
    if (depthFactor < 0.01f)
        return 0.0f;
    
    // Use world-space-like UVs based on depth for better projection
    float2 worldUV = uv * (10.0f + linearDepth * 0.5f);
    
    // Multiple overlapping caustic patterns
    float2 uv1 = worldUV + float2(time * 0.02f, time * 0.015f);
    float2 uv2 = worldUV * 1.3f - float2(time * 0.018f, -time * 0.022f);
    
    // Create caustic pattern using noise
    float c1 = noise(uv1 * 3.0f);
    float c2 = noise(uv2 * 2.5f);
    
    // Combine and sharpen
    float caustic = (c1 + c2) * 0.5f;
    caustic = pow(caustic, 3.0f) * 4.0f;  // Sharpen the caustic pattern
    
    return caustic * depthFactor;
}

// ============================================================================
// GOD RAYS - Volumetric light shafts from surface
// ============================================================================
static float3 calculateGodRays(float2 uv, float linearDepth, float2 sunPos, float time, float maxRayDepth)
{
    // Fade out with depth
    float visibility = 1.0f - saturate(linearDepth / maxRayDepth);
    visibility = pow(visibility, 0.5f);
    
    if (visibility < 0.01f)
        return float3(0, 0, 0);
    
    float2 toSun = sunPos - uv;
    float sunDist = length(toSun);
    float2 sunDir = toSun / max(sunDist, 0.001f);
    
    float rayAccum = 0.0f;
    const int NUM_SAMPLES = 16;
    float stepSize = min(sunDist, 0.5f) / float(NUM_SAMPLES);
    
    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        float2 samplePos = uv + sunDir * stepSize * float(i);
        float rayNoise = fbm(samplePos * 3.0f + float2(time * 0.03f, time * 0.02f), 3);
        float radialFalloff = 1.0f - saturate(float(i) / float(NUM_SAMPLES));
        rayAccum += rayNoise * radialFalloff * radialFalloff;
    }
    
    rayAccum = (rayAccum / float(NUM_SAMPLES)) * visibility;
    
    // Sun glow
    float sunGlow = pow(saturate(1.0f - sunDist * 1.5f), 4.0f) * 0.4f * visibility;
    rayAccum += sunGlow;
    
    return float3(0.4f, 0.7f, 0.9f) * rayAccum;
}

// ============================================================================
// PARTICLES - Floating debris/plankton
// ============================================================================
static float getParticles(float2 uv, float linearDepth, float time, float maxParticleDepth)
{
    float visibility = 1.0f - saturate(linearDepth / maxParticleDepth);
    visibility = pow(visibility, 0.7f);
    
    if (visibility < 0.01f)
        return 0.0f;
    
    float particles = 0.0f;
    
    for (int p = 0; p < 25; p++)
    {
        float seed = float(p);
        float pDepth = hash(float2(seed, seed + 10.0f));
        
        float speedMult = 0.3f + pDepth * 0.4f;
        float2 pPos = float2(
            frac(hash(float2(seed * 43.0f, seed)) + time * 0.02f * speedMult),
            frac(hash(float2(seed * 37.0f, seed + 5.0f)) + time * 0.03f * speedMult)
        );
        
        // Gentle swaying motion
        pPos.x += sin(time * 1.5f + seed * 2.0f) * 0.015f;
        pPos.y += cos(time * 1.2f + seed * 1.5f) * 0.01f;
        
        float size = 0.002f + hash(float2(seed + 5.0f, seed)) * 0.004f;
        float dist = length(uv - pPos);
        float particle = smoothstep(size, 0.0f, dist);
        
        particles += particle * (0.3f + pDepth * 0.5f);
    }
    
    return particles * visibility;
}

// ============================================================================
// BUBBLES - Rising from bottom of screen
// ============================================================================
static float getBubbles(float2 uv, float time)
{
    float bubbles = 0.0f;
    
    // Flip Y so bubbles rise from bottom
    float2 bubbleUV = float2(uv.x, 1.0f - uv.y);
    
    for (int b = 0; b < 12; b++)
    {
        float seed = float(b);
        float speed = 0.08f + hash(float2(seed, seed + 1.0f)) * 0.12f;
        
        // Y position cycles from 0 to 1 (bottom to top in flipped coords)
        float yOffset = frac(time * speed + hash(float2(seed * 7.0f, seed)));
        
        // Only show in lower 70% of screen (upper 70% in original coords)
        if (yOffset > 0.7f) continue;
        
        float2 bPos = float2(
            hash(float2(seed * 23.0f, seed + 3.0f)),
            yOffset
        );
        
        // Wobble as they rise
        bPos.x += sin(time * 2.5f + seed * 4.0f) * 0.025f * (1.0f - yOffset);
        
        float size = 0.004f + hash(float2(seed + 2.0f, seed)) * 0.006f;
        float dist = length(bubbleUV - bPos);
        
        // Bubble with highlight
        float bubble = smoothstep(size, size * 0.4f, dist);
        float highlight = smoothstep(size * 0.5f, 0.0f, length(bubbleUV - bPos + float2(size * 0.3f, -size * 0.3f)));
        
        bubbles += bubble * 0.5f + highlight * 0.8f;
    }
    
    return saturate(bubbles);
}

// ============================================================================
// MAIN SHADER
// ============================================================================
float4 main(
    PS_INPUT inPs,
    
    // Core
    uniform float underwaterFactor,
    uniform float timeSeconds,
    
    // Colors
    uniform float3 waterTint,
    uniform float3 deepWaterTint,
    
    // Fog
    uniform float fogDensity,
    uniform float maxFogDepth,
    
    // Effects
    uniform float absorptionScale,
    uniform float godRayStrength,
    uniform float causticStrength,
    uniform float maxCausticDepth,
    uniform float particleStrength,
    uniform float maxParticleDepth,
    uniform float bubbleStrength,
    
    // Visual
    uniform float distortion,
    uniform float contrast,
    uniform float saturation,
    uniform float vignette,
    uniform float chromaticAberration,
    
    // Sun
    uniform float2 sunScreenPos,
    
    // Depth reconstruction params (same as SSAO)
    uniform float2 projectionParams,
    uniform float farClipDistance
    
) : SV_Target0
{
    float2 uv = inPs.uv0;
    
    // ========================================================================
    // GET LINEAR DEPTH
    // ========================================================================
    float rawDepth = depthTexture.Sample(samplerPoint, uv).x;
    float linearDepth = getLinearDepth(rawDepth, projectionParams, farClipDistance);
    
    // Clamp to reasonable range
    linearDepth = clamp(linearDepth, 0.1f, farClipDistance);
    
    // ========================================================================
    // WAVE DISTORTION
    // ========================================================================
    // Stronger distortion, reduced slightly at distance
    float distortionFactor = 1.0f - saturate(linearDepth / maxFogDepth) * 0.3f;
    
    float wave1 = sin(uv.x * 15.0f + timeSeconds * 0.8f) * cos(uv.y * 12.0f - timeSeconds * 0.6f);
    float wave2 = cos(uv.x * 20.0f - timeSeconds * 0.7f) * sin(uv.y * 18.0f + timeSeconds * 0.75f);
    float wave3 = sin((uv.x + uv.y) * 25.0f + timeSeconds * 1.2f);
    
    float2 distortionVec = float2(
        wave1 * 0.5f + wave2 * 0.3f + wave3 * 0.2f,
        wave2 * 0.5f + wave1 * 0.3f + wave3 * 0.2f
    ) * distortion * underwaterFactor * distortionFactor;
    
    // ========================================================================
    // CHROMATIC ABERRATION
    // ========================================================================
    float2 centerOffset = uv - 0.5f;
    float aberrationAmount = chromaticAberration * length(centerOffset) * 2.0f * underwaterFactor;
    float2 aberrationDir = normalize(centerOffset + 0.001f);
    
    float2 uvR = uv + distortionVec + aberrationDir * aberrationAmount;
    float2 uvG = uv + distortionVec;
    float2 uvB = uv + distortionVec - aberrationDir * aberrationAmount;
    
    float r = rt0.Sample(samplerBilinear, uvR).r;
    float g = rt0.Sample(samplerBilinear, uvG).g;
    float b = rt0.Sample(samplerBilinear, uvB).b;
    float3 col = float3(r, g, b);
    
    // ========================================================================
    // LIGHT ABSORPTION (apply before fog)
    // ========================================================================
    col = applyLightAbsorption(col, linearDepth, absorptionScale * underwaterFactor);
    
    // ========================================================================
    // UNDERWATER FOG
    // ========================================================================
    col = calculateUnderwaterFog(
        col,
        linearDepth,
        waterTint,
        deepWaterTint,
        fogDensity * underwaterFactor,
        maxFogDepth
    );
    
    // ========================================================================
    // CAUSTICS (only on nearby surfaces)
    // ========================================================================
    float caustics = calculateCaustics(uv, linearDepth, timeSeconds, maxCausticDepth);
    col += caustics * causticStrength * underwaterFactor * float3(0.5f, 0.7f, 0.9f);
    
    // ========================================================================
    // GOD RAYS
    // ========================================================================
    float3 godRays = calculateGodRays(uv, linearDepth, sunScreenPos, timeSeconds, maxFogDepth * 0.5f);
    col += godRays * godRayStrength * underwaterFactor;
    
    // ========================================================================
    // PARTICLES
    // ========================================================================
    float particles = getParticles(uv, linearDepth, timeSeconds, maxParticleDepth);
    col += particles * particleStrength * underwaterFactor * float3(0.6f, 0.75f, 0.85f);
    
    // ========================================================================
    // BUBBLES
    // ========================================================================
    float bubbles = getBubbles(uv, timeSeconds);
    col += bubbles * bubbleStrength * underwaterFactor * float3(0.8f, 0.9f, 1.0f);
    
    // ========================================================================
    // COLOR GRADING - Shift towards blue/green
    // ========================================================================
    col.r *= lerp(1.0f, 0.7f, underwaterFactor);
    col.g *= lerp(1.0f, 0.9f, underwaterFactor);
    col.b *= lerp(1.0f, 1.1f, underwaterFactor);
    
    // ========================================================================
    // CONTRAST
    // ========================================================================
    float3 midGray = float3(0.5f, 0.5f, 0.5f);
    col = lerp(midGray, col, lerp(1.0f, contrast, underwaterFactor));
    
    // ========================================================================
    // SATURATION
    // ========================================================================
    col = applySaturation(col, lerp(1.0f, saturation, underwaterFactor));
    
    // ========================================================================
    // VIGNETTE
    // ========================================================================
    float2 vignetteUV = (uv - 0.5f) * 2.0f;
    float vignetteDist = length(vignetteUV);
    float vignetteAmount = 1.0f - pow(saturate(vignetteDist * 0.7f), vignette);
    col *= lerp(1.0f, vignetteAmount, underwaterFactor * 0.6f);
    
    return float4(saturate(col), 1.0f);
}
