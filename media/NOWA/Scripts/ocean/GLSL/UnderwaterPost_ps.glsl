// ============================================================================
// UNDERWATER POST-PROCESS SHADER WITH DEPTH TEXTURE (GLSL)
// Inspired by Hydrax underwater effects for Ogre-Next
// ============================================================================

#version 330

// Textures
uniform sampler2D rt0;          // Scene color
uniform sampler2D depthTexture; // Depth buffer

// Core parameters
uniform float underwaterFactor;
uniform float timeSeconds;

// Water colors
uniform vec3 waterTint;
uniform vec3 deepWaterTint;

// Visual effects
uniform float distortion;
uniform float contrast;
uniform float saturation;
uniform float vignette;

// Sun/Light
uniform vec2 sunScreenPos;

// Fog parameters
uniform float fogDensity;
uniform float fogStart;
uniform float maxFogDepth;

// God rays
uniform float godRayStrength;
uniform float godRayDensity;
uniform float maxGodRayDepth;

// Caustics
uniform float causticStrength;
uniform float maxCausticDepth;

// Absorption
uniform float absorptionScale;

// Particles & Bubbles
uniform float particleStrength;
uniform float maxParticleDepth;
uniform float bubbleStrength;

// Scattering
uniform float scatterDensity;
uniform vec3 scatterColor;

// Other
uniform float chromaticAberration;

// Camera parameters
uniform float nearClipDistance;
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
// DEPTH CONVERSION
// ============================================================================
float linearizeDepth(float rawDepth, float nearClip, float farClip)
{
    // For reversed-Z (common in Ogre-Next)
    float z = rawDepth;
    return (nearClip * farClip) / (farClip - z * (farClip - nearClip));
}

// ============================================================================
// UNDERWATER FOG
// ============================================================================
vec3 calculateUnderwaterFog(
    vec3 sceneColor,
    float depth,
    vec3 waterColor,
    vec3 deepWaterColor,
    float density,
    float start,
    float maxDepth
)
{
    float fogDistance = max(0.0, depth - start);
    float fogFactor = 1.0 - exp(-density * fogDistance);
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    
    float depthRatio = clamp(depth / maxDepth, 0.0, 1.0);
    vec3 fogColor = mix(waterColor, deepWaterColor, depthRatio * depthRatio);
    
    return mix(sceneColor, fogColor, fogFactor);
}

// ============================================================================
// LIGHT ABSORPTION
// ============================================================================
vec3 applyLightAbsorption(vec3 color, float depth, float scale)
{
    vec3 absorption = vec3(0.45, 0.18, 0.06) * scale;
    vec3 transmitted = exp(-absorption * depth);
    return color * transmitted;
}

// ============================================================================
// CAUSTICS
// ============================================================================
float calculateCaustics(vec2 uv, float depth, float time, float maxDepth)
{
    float causticIntensity = 1.0 - clamp(depth / maxDepth, 0.0, 1.0);
    causticIntensity = pow(causticIntensity, 1.5);
    
    if (causticIntensity < 0.01)
        return 0.0;
    
    vec2 causticUV1 = uv * 8.0 + vec2(time * 0.03, time * 0.02);
    vec2 causticUV2 = uv * 12.0 - vec2(time * 0.025, -time * 0.035);
    vec2 causticUV3 = uv * 6.0 + vec2(-time * 0.02, time * 0.015);
    
    float c1 = abs(sin(causticUV1.x * 3.14159) * cos(causticUV1.y * 3.14159));
    float c2 = abs(sin(causticUV2.x * 2.5) * cos(causticUV2.y * 2.8));
    float c3 = noise(causticUV3 * 2.0);
    
    float caustic = (c1 * 0.4 + c2 * 0.35 + c3 * 0.25);
    caustic = pow(caustic, 2.0) * 2.0;
    
    return caustic * causticIntensity;
}

// ============================================================================
// GOD RAYS
// ============================================================================
vec3 calculateGodRays(
    vec2 uv,
    float depth,
    vec2 sunPos,
    float time,
    float maxDepth,
    float density
)
{
    float rayVisibility = 1.0 - clamp(depth / maxDepth, 0.0, 1.0);
    rayVisibility = pow(rayVisibility, 0.7);
    
    if (rayVisibility < 0.01)
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
        
        float rayNoise = fbm(samplePos * 4.0 + vec2(time * 0.05, 0.0), 2);
        rayNoise = pow(rayNoise, 1.5);
        
        float radialFalloff = 1.0 - clamp(float(i) / float(NUM_SAMPLES), 0.0, 1.0);
        radialFalloff = pow(radialFalloff, 0.5);
        
        rayAccum += rayNoise * radialFalloff * density;
    }
    
    rayAccum /= float(NUM_SAMPLES);
    rayAccum *= rayVisibility;
    
    float sunGlow = pow(clamp(1.0 - sunDist * 1.2, 0.0, 1.0), 3.0);
    rayAccum += sunGlow * 0.3 * rayVisibility;
    
    vec3 rayColor = vec3(0.3, 0.6, 0.8);
    
    return rayColor * rayAccum;
}

// ============================================================================
// PARTICLES
// ============================================================================
float getParticles(vec2 uv, float depth, float time, float maxDepth)
{
    float particleVisibility = 1.0 - clamp(depth / maxDepth, 0.0, 1.0);
    
    if (particleVisibility < 0.01)
        return 0.0;
    
    float particles = 0.0;
    
    for (int p = 0; p < 40; p++)
    {
        float seed = float(p);
        float particleDepth = hash(vec2(seed, seed + 10.0));
        
        float speedMult = 0.5 + particleDepth * 0.5;
        vec2 pPos = vec2(
            fract(sin(seed * 43.0) * 1000.0 + time * 0.05 * speedMult),
            fract(cos(seed * 37.0) * 1000.0 + time * 0.08 * speedMult + seed * 0.1)
        );
        
        pPos.x += sin(time * 2.0 + seed) * 0.02 * particleDepth;
        
        float size = (0.0015 + hash(vec2(seed + 5.0, seed)) * 0.0025) * (0.6 + particleDepth * 0.4);
        
        float dist = length(uv - pPos);
        float particle = smoothstep(size, 0.0, dist);
        
        particles += particle * (0.4 + particleDepth * 0.6);
    }
    
    return particles * particleVisibility;
}

// ============================================================================
// BUBBLES
// ============================================================================
float getBubbles(vec2 uv, float time)
{
    float bubbles = 0.0;
    
    for (int b = 0; b < 20; b++)
    {
        float seed = float(b);
        float speed = 0.12 + hash(vec2(seed, seed + 1.0)) * 0.18;
        
        float yOffset = fract(time * speed + seed * 0.08);
        vec2 bPos = vec2(
            fract(sin(seed * 23.0) * 1000.0),
            yOffset
        );
        
        bPos.x += sin(time * 3.0 + seed * 6.28) * 0.035;
        bPos.x += sin(time * 5.5 + seed * 3.14) * 0.022;
        
        float size = 0.004 + hash(vec2(seed + 2.0, seed)) * 0.005;
        float dist = length(uv - bPos);
        
        float bubble = smoothstep(size, size * 0.3, dist) - smoothstep(size * 1.2, size, dist);
        float highlight = smoothstep(size * 0.4, 0.0, 
                                     length(uv - bPos + vec2(size * 0.4, -size * 0.4)));
        
        bubbles += bubble * 0.7 + highlight * 1.8;
    }
    
    return clamp(bubbles, 0.0, 1.0);
}

// ============================================================================
// LIGHT SCATTERING
// ============================================================================
vec3 applyScattering(
    vec3 color,
    float depth,
    vec3 scatColor,
    float scatDensity,
    vec2 uv,
    vec2 sunPos
)
{
    vec2 toSun = normalize(sunPos - uv);
    float sunAlignment = dot(toSun, vec2(0.0, -1.0)) * 0.5 + 0.5;
    
    float scatter = 1.0 - exp(-scatDensity * depth);
    scatter *= (0.5 + sunAlignment * 0.5);
    
    return mix(color, scatColor, clamp(scatter, 0.0, 1.0));
}

// ============================================================================
// MAIN
// ============================================================================
void main()
{
    vec2 uv = inPs.uv0;
    
    // Sample depth and linearize
    float rawDepth = texture(depthTexture, uv).r;
    float linearDepth = linearizeDepth(rawDepth, nearClipDistance, farClipDistance);
    linearDepth = clamp(linearDepth, 0.0, farClipDistance);
    
    // Wave distortion (depth-modulated)
    float distortionDepthFactor = 1.0 - clamp(linearDepth / maxFogDepth, 0.0, 1.0) * 0.5;
    
    float bigWave1 = sin(uv.x * 20.0 + timeSeconds * 0.4) * cos(uv.y * 15.0 - timeSeconds * 0.3);
    float bigWave2 = cos(uv.x * 25.0 - timeSeconds * 0.35) * sin(uv.y * 20.0 + timeSeconds * 0.38);
    float medWave1 = sin(uv.x * 35.0 + timeSeconds * 0.6) * cos(uv.y * 30.0);
    float medWave2 = cos(uv.x * 40.0) * sin(uv.y * 35.0 - timeSeconds * 0.55);
    float smallWave = sin(uv.x * 50.0 + uv.y * 45.0 + timeSeconds * 0.8);
    float turbulence = fbm(uv * 10.0 + vec2(timeSeconds * 0.12, -timeSeconds * 0.1), 3) - 0.5;
    
    vec2 distortionVec = vec2(
        bigWave1 * 0.35 + medWave1 * 0.25 + smallWave * 0.15 + turbulence * 0.25,
        bigWave2 * 0.35 + medWave2 * 0.25 + smallWave * 0.15 + turbulence * 0.25
    ) * distortion * underwaterFactor * distortionDepthFactor;
    
    // Chromatic aberration
    vec2 aberrationDir = normalize(uv - 0.5);
    vec2 aberration = aberrationDir * chromaticAberration * underwaterFactor;
    
    float r = texture(rt0, uv + distortionVec + aberration).r;
    float g = texture(rt0, uv + distortionVec).g;
    float b = texture(rt0, uv + distortionVec - aberration).b;
    vec3 col = vec3(r, g, b);
    
    // Light absorption
    col = applyLightAbsorption(col, linearDepth, absorptionScale * underwaterFactor);
    
    // Underwater fog
    col = calculateUnderwaterFog(
        col,
        linearDepth,
        waterTint,
        deepWaterTint,
        fogDensity * underwaterFactor,
        fogStart,
        maxFogDepth
    );
    
    // Caustics
    float caustics = calculateCaustics(uv, linearDepth, timeSeconds, maxCausticDepth);
    col += caustics * causticStrength * underwaterFactor * vec3(0.6, 0.8, 1.0);
    
    // God rays
    vec3 godRays = calculateGodRays(
        uv,
        linearDepth,
        sunScreenPos,
        timeSeconds,
        maxGodRayDepth,
        godRayDensity
    );
    col += godRays * godRayStrength * underwaterFactor;
    
    // Light scattering
    col = applyScattering(
        col,
        linearDepth,
        scatterColor,
        scatterDensity * underwaterFactor,
        uv,
        sunScreenPos
    );
    
    // Particles
    float particles = getParticles(uv, linearDepth, timeSeconds, maxParticleDepth);
    col += particles * particleStrength * underwaterFactor * vec3(0.55, 0.68, 0.78);
    
    // Bubbles
    float bubbles = getBubbles(uv, timeSeconds);
    col += bubbles * bubbleStrength * underwaterFactor * vec3(0.7, 0.88, 1.0);
    
    // Color grading (depth-based)
    float depthColorShift = clamp(linearDepth / maxFogDepth, 0.0, 1.0);
    col.r *= mix(1.0, 0.5, depthColorShift * underwaterFactor);
    col.g *= mix(1.0, 0.75, depthColorShift * underwaterFactor);
    col.b *= mix(1.0, 1.1, depthColorShift * underwaterFactor);
    
    // Contrast
    col = (col - 0.5) * mix(1.0, contrast, underwaterFactor) + 0.5;
    
    // Saturation
    col = applySaturation(col, mix(1.0, saturation, underwaterFactor));
    
    // Vignette
    vec2 vignetteUV = (uv - 0.5) * 1.9;
    float vignetteAmount = pow(clamp(1.0 - dot(vignetteUV, vignetteUV) * 0.35, 0.0, 1.0), vignette);
    col *= mix(1.0, vignetteAmount, underwaterFactor);
    
    fragColour = vec4(clamp(col, 0.0, 1.0), 1.0);
}
