// ============================================================================
// UNDERWATER POST-PROCESS SHADER WITH DEPTH TEXTURE (GLSL)
// Uses Ogre's auto-bound projection parameters
// ============================================================================

#version 330

// Textures
uniform sampler2D rt0;          // Scene color
uniform sampler2D depthTexture; // Depth buffer

// Core
uniform float underwaterFactor;
uniform float timeSeconds;

// Colors
uniform vec3 waterTint;
uniform vec3 deepWaterTint;

// Fog
uniform float fogDensity;
uniform float maxFogDepth;

// Effects
uniform float absorptionScale;
uniform float godRayStrength;
uniform float causticStrength;
uniform float maxCausticDepth;
uniform float particleStrength;
uniform float maxParticleDepth;
uniform float bubbleStrength;

// Visual
uniform float distortion;
uniform float contrast;
uniform float saturation;
uniform float vignette;
uniform float chromaticAberration;

// Sun
uniform vec2 sunScreenPos;

// Depth reconstruction params (same as SSAO)
uniform vec2 projectionParams;
uniform float farClipDistance;

// Input/Output
in block
{
    vec2 uv0;
} inPs;

out vec4 fragColour;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

vec3 applySaturation(vec3 c, float s)
{
    float l = dot(c, vec3(0.2126, 0.7152, 0.0722));
    return mix(vec3(l), c, s);
}

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p, int octaves)
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
float getLinearDepth(float rawDepth, vec2 projParams, float farClip)
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
vec3 calculateUnderwaterFog(
    vec3 sceneColor,
    float linearDepth,
    vec3 waterColor,
    vec3 deepWaterColor,
    float density,
    float maxDepth
)
{
    // Normalize depth for color blending
    float normalizedDepth = clamp(linearDepth / maxDepth, 0.0, 1.0);

    // Exponential fog - more realistic
    float fogFactor = 1.0 - exp(-density * linearDepth);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    // Depth-based fog color transition
    vec3 fogColor = mix(waterColor, deepWaterColor, pow(normalizedDepth, 0.8));

    return mix(sceneColor, fogColor, fogFactor);
}

// ============================================================================
// LIGHT ABSORPTION - Red absorbs first, blue travels furthest
// ============================================================================
vec3 applyLightAbsorption(vec3 color, float linearDepth, float absScale)
{
    // Absorption coefficients: red > green > blue
    vec3 absorption = vec3(0.4, 0.15, 0.05) * absScale;
    vec3 transmitted = exp(-absorption * linearDepth);
    // Keep minimum color to avoid total blackout
    transmitted = max(transmitted, vec3(0.1, 0.2, 0.4));
    return color * transmitted;
}

// ============================================================================
// CAUSTICS - Light patterns on surfaces (only on geometry, not in water volume)
// ============================================================================
float calculateCaustics(vec2 uv, float linearDepth, float time, float maxDepth)
{
    // Only show caustics on nearby surfaces
    float depthFactor = 1.0 - clamp(linearDepth / maxDepth, 0.0, 1.0);
    depthFactor = pow(depthFactor, 2.5);  // Sharper falloff

    if (depthFactor < 0.01)
        return 0.0;

    // Use world-space-like UVs based on depth for better projection
    vec2 worldUV = uv * (10.0 + linearDepth * 0.5);

    // Multiple overlapping caustic patterns
    vec2 uv1 = worldUV + vec2(time * 0.02, time * 0.015);
    vec2 uv2 = worldUV * 1.3 - vec2(time * 0.018, -time * 0.022);

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
vec3 calculateGodRays(vec2 uv, float linearDepth, vec2 sunPos, float time, float maxRayDepth)
{
    // Fade out with depth
    float visibility = 1.0 - clamp(linearDepth / maxRayDepth, 0.0, 1.0);
    visibility = pow(visibility, 0.5);

    if (visibility < 0.01)
        return vec3(0.0);

    vec2 toSun = sunPos - uv;
    float sunDist = length(toSun);
    vec2 sunDir = toSun / max(sunDist, 0.001);

    float rayAccum = 0.0;
    const int NUM_SAMPLES = 16;
    float stepSize = min(sunDist, 0.5) / float(NUM_SAMPLES);

    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        vec2 samplePos = uv + sunDir * stepSize * float(i);
        float rayNoise = fbm(samplePos * 3.0 + vec2(time * 0.03, time * 0.02), 3);
        float radialFalloff = 1.0 - clamp(float(i) / float(NUM_SAMPLES), 0.0, 1.0);
        rayAccum += rayNoise * radialFalloff * radialFalloff;
    }

    rayAccum = (rayAccum / float(NUM_SAMPLES)) * visibility;

    // Sun glow
    float sunGlow = pow(clamp(1.0 - sunDist * 1.5, 0.0, 1.0), 4.0) * 0.4 * visibility;
    rayAccum += sunGlow;

    return vec3(0.4, 0.7, 0.9) * rayAccum;
}

// ============================================================================
// PARTICLES - Floating debris/plankton
// ============================================================================
float getParticles(vec2 uv, float linearDepth, float time, float maxDepth)
{
    float visibility = 1.0 - clamp(linearDepth / maxDepth, 0.0, 1.0);
    visibility = pow(visibility, 0.7);

    if (visibility < 0.01)
        return 0.0;

    float particles = 0.0;

    for (int p = 0; p < 25; p++)
    {
        float seed = float(p);
        float pDepth = hash(vec2(seed, seed + 10.0));

        float speedMult = 0.3 + pDepth * 0.4;
        vec2 pPos = vec2(
            fract(hash(vec2(seed * 43.0, seed)) + time * 0.02 * speedMult),
            fract(hash(vec2(seed * 37.0, seed + 5.0)) + time * 0.03 * speedMult)
        );

        // Gentle swaying motion
        pPos.x += sin(time * 1.5 + seed * 2.0) * 0.015;
        pPos.y += cos(time * 1.2 + seed * 1.5) * 0.01;

        float size = 0.002 + hash(vec2(seed + 5.0, seed)) * 0.004;
        float dist = length(uv - pPos);
        float particle = smoothstep(size, 0.0, dist);

        particles += particle * (0.3 + pDepth * 0.5);
    }

    return particles * visibility;
}

// ============================================================================
// BUBBLES - Rising from bottom of screen
// ============================================================================
float getBubbles(vec2 uv, float time)
{
    float bubbles = 0.0;

    // Flip Y so bubbles rise from bottom
    vec2 bubbleUV = vec2(uv.x, 1.0 - uv.y);

    for (int b = 0; b < 12; b++)
    {
        float seed = float(b);
        float speed = 0.08 + hash(vec2(seed, seed + 1.0)) * 0.12;

        // Y position cycles from 0 to 1 (bottom to top in flipped coords)
        float yOffset = fract(time * speed + hash(vec2(seed * 7.0, seed)));

        // Only show in lower 70% of screen (upper 70% in original coords)
        if (yOffset > 0.7) continue;

        vec2 bPos = vec2(
            hash(vec2(seed * 23.0, seed + 3.0)),
            yOffset
        );

        // Wobble as they rise
        bPos.x += sin(time * 2.5 + seed * 4.0) * 0.025 * (1.0 - yOffset);

        float size = 0.004 + hash(vec2(seed + 2.0, seed)) * 0.006;
        float dist = length(bubbleUV - bPos);

        // Bubble with highlight
        float bubble = smoothstep(size, size * 0.4, dist);
        float highlight = smoothstep(size * 0.5, 0.0, length(bubbleUV - bPos + vec2(size * 0.3, -size * 0.3)));

        bubbles += bubble * 0.5 + highlight * 0.8;
    }

    return clamp(bubbles, 0.0, 1.0);
}

// ============================================================================
// MAIN SHADER
// ============================================================================
void main()
{
    vec2 uv = inPs.uv0;

    // ========================================================================
    // GET LINEAR DEPTH
    // ========================================================================
    float rawDepth = texture(depthTexture, uv).x;
    float linearDepth = getLinearDepth(rawDepth, projectionParams, farClipDistance);

    // Clamp to reasonable range
    linearDepth = clamp(linearDepth, 0.1, farClipDistance);

    // ========================================================================
    // WAVE DISTORTION
    // ========================================================================
    // Stronger distortion, reduced slightly at distance
    float distortionFactor = 1.0 - clamp(linearDepth / maxFogDepth, 0.0, 1.0) * 0.3;

    float wave1 = sin(uv.x * 15.0 + timeSeconds * 0.8) * cos(uv.y * 12.0 - timeSeconds * 0.6);
    float wave2 = cos(uv.x * 20.0 - timeSeconds * 0.7) * sin(uv.y * 18.0 + timeSeconds * 0.75);
    float wave3 = sin((uv.x + uv.y) * 25.0 + timeSeconds * 1.2);

    vec2 distortionVec = vec2(
        wave1 * 0.5 + wave2 * 0.3 + wave3 * 0.2,
        wave2 * 0.5 + wave1 * 0.3 + wave3 * 0.2
    ) * distortion * underwaterFactor * distortionFactor;

    // ========================================================================
    // CHROMATIC ABERRATION
    // ========================================================================
    vec2 centerOffset = uv - 0.5;
    float aberrationAmount = chromaticAberration * length(centerOffset) * 2.0 * underwaterFactor;
    vec2 aberrationDir = normalize(centerOffset + 0.001);

    vec2 uvR = uv + distortionVec + aberrationDir * aberrationAmount;
    vec2 uvG = uv + distortionVec;
    vec2 uvB = uv + distortionVec - aberrationDir * aberrationAmount;

    float r = texture(rt0, uvR).r;
    float g = texture(rt0, uvG).g;
    float b = texture(rt0, uvB).b;
    vec3 col = vec3(r, g, b);

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
    col += caustics * causticStrength * underwaterFactor * vec3(0.5, 0.7, 0.9);

    // ========================================================================
    // GOD RAYS
    // ========================================================================
    vec3 godRays = calculateGodRays(uv, linearDepth, sunScreenPos, timeSeconds, maxFogDepth * 0.5);
    col += godRays * godRayStrength * underwaterFactor;

    // ========================================================================
    // PARTICLES
    // ========================================================================
    float particles = getParticles(uv, linearDepth, timeSeconds, maxParticleDepth);
    col += particles * particleStrength * underwaterFactor * vec3(0.6, 0.75, 0.85);

    // ========================================================================
    // BUBBLES
    // ========================================================================
    float bubbles = getBubbles(uv, timeSeconds);
    col += bubbles * bubbleStrength * underwaterFactor * vec3(0.8, 0.9, 1.0);

    // ========================================================================
    // COLOR GRADING - Shift towards blue/green
    // ========================================================================
    col.r *= mix(1.0, 0.7, underwaterFactor);
    col.g *= mix(1.0, 0.9, underwaterFactor);
    col.b *= mix(1.0, 1.1, underwaterFactor);

    // ========================================================================
    // CONTRAST
    // ========================================================================
    vec3 midGray = vec3(0.5);
    col = mix(midGray, col, mix(1.0, contrast, underwaterFactor));

    // ========================================================================
    // SATURATION
    // ========================================================================
    col = applySaturation(col, mix(1.0, saturation, underwaterFactor));

    // ========================================================================
    // VIGNETTE
    // ========================================================================
    vec2 vignetteUV = (uv - 0.5) * 2.0;
    float vignetteDist = length(vignetteUV);
    float vignetteAmount = 1.0 - pow(clamp(vignetteDist * 0.7, 0.0, 1.0), vignette);
    col *= mix(1.0, vignetteAmount, underwaterFactor * 0.6);

    fragColour = vec4(clamp(col, 0.0, 1.0), 1.0);
}
