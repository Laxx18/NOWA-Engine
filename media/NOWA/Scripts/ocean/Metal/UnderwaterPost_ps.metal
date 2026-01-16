#include <metal_stdlib>
using namespace metal;

struct PS_INPUT
{
    float4 position [[position]];
    float2 uv0;
};

struct Uniforms
{
    float underwaterFactor;
    float timeSeconds;
    float3 waterTint;
    float3 deepWaterTint;
    float distortion;
    float contrast;
    float saturation;
    float vignette;
    float2 sunScreenPos;
    float fogStrength;
    float lightGlowStrength;
    float particleStrength;
    float bubbleStrength;
    float chromaticAberration;
    float turbidity;
};

// Apply saturation adjustment
float3 applySaturation(float3 c, float s)
{
    float l = dot(c, float3(0.2126, 0.7152, 0.0722));
    return mix(float3(l), c, s);
}

// Hash function for procedural noise
float hash(float2 p)
{
    return fract(sin(dot(p, float2(127.1, 311.7))) * 43758.5453);
}

// Improved noise function
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

// Fractal Brownian Motion
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

// Simple directional light glow instead of god rays
float3 getDirectionalGlow(float2 uv, float2 lightPos, float time)
{
    // Distance from light source
    float2 toLight = lightPos - uv;
    float dist = length(toLight);
    
    // Simple radial gradient
    float glow = pow(saturate(1.0 - dist * 0.8), 3.0);
    
    // Add subtle animation
    float pulse = sin(time * 0.3) * 0.1 + 0.9;
    glow *= pulse;
    
    // Add some noise to break up the gradient
    float glowNoise = fbm(uv * 4.0 + time * 0.05, 2) * 0.3 + 0.7;
    glow *= glowNoise;
    
    return float3(glow);
}

// Particles
float getParticles(float2 uv, float time)
{
    float particles = 0.0;
    
    for (int p = 0; p < 40; p++)
    {
        float seed = float(p);
        float depth = hash(float2(seed, seed + 10.0));
        
        float speedMult = 0.5 + depth * 0.5;
        float2 pPos = float2(
            fract(sin(seed * 43.0) * 1000.0 + time * 0.05 * speedMult),
            fract(cos(seed * 37.0) * 1000.0 + time * 0.08 * speedMult + seed * 0.1)
        );
        
        pPos.x += sin(time * 2.0 + seed) * 0.02 * depth;
        
        float size = (0.0015 + hash(float2(seed + 5.0, seed)) * 0.0025) * (0.6 + depth * 0.4);
        
        float dist = length(uv - pPos);
        float particle = smoothstep(size, 0.0, dist);
        
        particles += particle * (0.4 + depth * 0.6);
    }
    
    return particles;
}

// Bubbles
float getBubbles(float2 uv, float time)
{
    float bubbles = 0.0;
    
    for (int b = 0; b < 20; b++)
    {
        float seed = float(b);
        float speed = 0.12 + hash(float2(seed, seed + 1.0)) * 0.18;
        
        float yOffset = fract(time * speed + seed * 0.08);
        float2 bPos = float2(
            fract(sin(seed * 23.0) * 1000.0),
            yOffset
        );
        
        bPos.x += sin(time * 3.0 + seed * 6.28) * 0.035;
        bPos.x += sin(time * 5.5 + seed * 3.14) * 0.022;
        
        float size = 0.004 + hash(float2(seed + 2.0, seed)) * 0.005;
        float dist = length(uv - bPos);
        
        float bubble = smoothstep(size, size * 0.3, dist) - smoothstep(size * 1.2, size, dist);
        float highlight = smoothstep(size * 0.4, 0.0, 
                                     length(uv - bPos + float2(size * 0.4, -size * 0.4)));
        
        bubbles += bubble * 0.7 + highlight * 1.8;
    }
    
    return saturate(bubbles);
}

fragment float4 underwaterPost_fragment(
    PS_INPUT inPs [[stage_in]],
    texture2d<float> rt0 [[texture(0)]],
    sampler samp0 [[sampler(0)]],
    constant Uniforms& uniforms [[buffer(0)]]
)
{
    float2 uv = inPs.uv0;
    
    // === VERY STRONG WAVE DISTORTION ===
    // Layer 1: Large slow waves
    float bigWave1 = sin(uv.x * 20.0 + uniforms.timeSeconds * 0.4) * cos(uv.y * 15.0 - uniforms.timeSeconds * 0.3);
    float bigWave2 = cos(uv.x * 25.0 - uniforms.timeSeconds * 0.35) * sin(uv.y * 20.0 + uniforms.timeSeconds * 0.38);
    
    // Layer 2: Medium waves
    float medWave1 = sin(uv.x * 35.0 + uniforms.timeSeconds * 0.6) * cos(uv.y * 30.0);
    float medWave2 = cos(uv.x * 40.0) * sin(uv.y * 35.0 - uniforms.timeSeconds * 0.55);
    
    // Layer 3: Small fast waves
    float smallWave = sin(uv.x * 50.0 + uv.y * 45.0 + uniforms.timeSeconds * 0.8);
    
    // Layer 4: Organic turbulence
    float turbulence = fbm(uv * 10.0 + float2(uniforms.timeSeconds * 0.12, -uniforms.timeSeconds * 0.1), 3) - 0.5;
    
    // Combine all wave layers
    float2 distortionVec = float2(
        bigWave1 * 0.35 + medWave1 * 0.25 + smallWave * 0.15 + turbulence * 0.25,
        bigWave2 * 0.35 + medWave2 * 0.25 + smallWave * 0.15 + turbulence * 0.25
    ) * uniforms.distortion * uniforms.underwaterFactor;
    
    // === CHROMATIC ABERRATION ===
    float2 aberrationDir = normalize(uv - 0.5);
    float2 aberration = aberrationDir * uniforms.chromaticAberration * uniforms.underwaterFactor;
    
    float r = rt0.sample(samp0, uv + distortionVec + aberration).r;
    float g = rt0.sample(samp0, uv + distortionVec).g;
    float b = rt0.sample(samp0, uv + distortionVec - aberration).b;
    float3 col = float3(r, g, b);
    
    // === IMPROVED FAKE FOG (NO DEPTH BUFFER) - BALANCED VERSION ===
    
    // Calculate scene brightness/luminance
    float brightness = dot(col, float3(0.299, 0.587, 0.114));
    
    // Brightness-based fog (REDUCED - only affects darker/distant pixels)
    float brightnessFog = pow(1.0 - saturate(brightness * 0.8), 3.5);
    
    // Radial distance from screen center
    float2 centerOffset = uv - 0.5;
    float radialDist = length(centerOffset);
    float radialFog = pow(radialDist * 1.4, 4.0);
    
    // Vertical gradient (bottom = deeper = more fog) - REDUCED
    float verticalFog = pow(1.0 - uv.y, 4.5) * 0.6;
    
    // Animated volumetric fog layers - SUBTLE
    float2 fogFlow1 = uv * 2.5 + float2(uniforms.timeSeconds * 0.018, -uniforms.timeSeconds * 0.022);
    float2 fogFlow2 = uv * 4.5 - float2(uniforms.timeSeconds * 0.015, uniforms.timeSeconds * 0.025);
    float2 fogFlow3 = uv * 7.0 + float2(-uniforms.timeSeconds * 0.012, uniforms.timeSeconds * 0.018);
    
    float volumeFog = 
        fbm(fogFlow1, 4) * 0.2 +
        fbm(fogFlow2, 3) * 0.15 +
        fbm(fogFlow3, 2) * 0.1;
    
    // Edge darkness detector - REDUCED
    float edgeDarkness = 1.0 - saturate(brightness * 1.5);
    float edgeFogBoost = pow(edgeDarkness, 2.0) * radialFog * 0.5;
    
    // === COMBINE ALL FOG LAYERS - MUCH MORE CONSERVATIVE ===
    float totalFog = saturate(
        brightnessFog * 0.35 +
        radialFog * 0.15 +
        verticalFog * 0.25 +
        volumeFog * 0.15 +
        edgeFogBoost * 0.2
    );
    
    // GENTLER fog buildup (higher power = slower fog increase)
    totalFog = pow(totalFog, 1.2);
    
    // Multi-layer fog colors for depth perception - LESS SATURATED
    float3 nearFogColor = uniforms.waterTint * 1.3;
    float3 midFogColor = uniforms.deepWaterTint * 1.4;
    float3 farFogColor = uniforms.deepWaterTint * 1.0;
    
    // Lerp between fog layers based on fog amount
    float3 fogColor = mix(nearFogColor, midFogColor, saturate(totalFog * 1.2));
    fogColor = mix(fogColor, farFogColor, saturate(totalFog * totalFog * 1.5));
    
    // Apply fog with MUCH GENTLER strength
    float finalFogAmount = totalFog * uniforms.fogStrength * uniforms.underwaterFactor;
    col = mix(col, fogColor, saturate(finalFogAmount * 0.8));
    
    // === DARKEN DISTANT AREAS - GENTLER ===
    float distanceDarkening = pow(totalFog, 2.5);
    col *= mix(1.0, 0.5, distanceDarkening * uniforms.underwaterFactor);
    
    // === SIMPLE DIRECTIONAL LIGHT GLOW (replaces god rays) ===
    float3 lightGlow = getDirectionalGlow(uv, uniforms.sunScreenPos, uniforms.timeSeconds);
    col += lightGlow * uniforms.lightGlowStrength * uniforms.underwaterFactor *
           float3(0.4, 0.65, 0.85);
    
    // === PARTICLES ===
    float particles = getParticles(uv, uniforms.timeSeconds);
    col += particles * uniforms.particleStrength * uniforms.underwaterFactor * 
           float3(0.55, 0.68, 0.78);
    
    // === BUBBLES ===
    float bubbles = getBubbles(uv, uniforms.timeSeconds);
    col += bubbles * uniforms.bubbleStrength * uniforms.underwaterFactor * 
           float3(0.7, 0.88, 1.0);
    
    // === EDGE DARKENING ===
    float edgeDarkening = pow(radialDist * 1.5, 2.5);
    float edgeTint = saturate(edgeDarkening * 0.7 + totalFog * 0.3);
    col = mix(col, uniforms.waterTint * 1.4, edgeTint * uniforms.underwaterFactor);
    
    // === DEPTH GRADIENT ===
    float depthGradient = pow(1.0 - uv.y, 2.5);
    col = mix(col, uniforms.deepWaterTint * 1.3, depthGradient * 0.4 * uniforms.underwaterFactor);
    
    // === COLOR GRADING WITH DISTANCE ===
    // Base color grading
    col.r *= mix(1.0, 0.68, uniforms.underwaterFactor);
    col.g *= mix(1.0, 0.9, uniforms.underwaterFactor);
    col.b *= mix(1.0, 1.25, uniforms.underwaterFactor);
    
    // Additional color shift with distance - MUCH GENTLER
    col.r *= mix(1.0, 0.65, totalFog * uniforms.underwaterFactor);
    col.g *= mix(1.0, 0.8, totalFog * uniforms.underwaterFactor);
    col.b *= mix(1.0, 1.05, totalFog * uniforms.underwaterFactor);
    
    // === CONTRAST ===
    col = (col - 0.5) * mix(1.0, uniforms.contrast, uniforms.underwaterFactor) + 0.5;
    
    // === SATURATION ===
    col = applySaturation(col, mix(1.0, uniforms.saturation, uniforms.underwaterFactor));
    
    // === VIGNETTE ===
    float2 vignetteUV = (uv - 0.5) * 1.9;
    float vignetteAmount = pow(saturate(1.0 - dot(vignetteUV, vignetteUV) * 0.35), uniforms.vignette);
    col *= mix(1.0, vignetteAmount, uniforms.underwaterFactor);
    
    // === TURBIDITY NOISE ===
    float turbNoise = fbm(uv * 7.0 + uniforms.timeSeconds * 0.022, 3) * 0.045;
    col += turbNoise * uniforms.underwaterFactor * float3(0.32, 0.42, 0.52);
    
    return float4(saturate(col), 1.0);
}