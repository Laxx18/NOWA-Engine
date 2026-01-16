#version 300 es
precision highp float;

uniform sampler2D rt0;

in vec2 uv0;
out vec4 fragColor;

// Uniforms
uniform float underwaterFactor;
uniform float timeSeconds;
uniform vec3 waterTint;
uniform vec3 deepWaterTint;
uniform float distortion;
uniform float contrast;
uniform float saturation;
uniform float vignette;
uniform vec2 sunScreenPos;
uniform float fogStrength;
uniform float lightGlowStrength;
uniform float particleStrength;
uniform float bubbleStrength;
uniform float chromaticAberration;
uniform float turbidity;

// Apply saturation adjustment
vec3 applySaturation(vec3 c, float s)
{
    float l = dot(c, vec3(0.2126, 0.7152, 0.0722));
    return mix(vec3(l), c, s);
}

// Hash function for procedural noise
float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// Improved noise function
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

// Fractal Brownian Motion
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

// Simple directional light glow instead of god rays
vec3 getDirectionalGlow(vec2 uv, vec2 lightPos, float time)
{
    // Distance from light source
    vec2 toLight = lightPos - uv;
    float dist = length(toLight);
    
    // Simple radial gradient
    float glow = pow(clamp(1.0 - dist * 0.8, 0.0, 1.0), 3.0);
    
    // Add subtle animation
    float pulse = sin(time * 0.3) * 0.1 + 0.9;
    glow *= pulse;
    
    // Add some noise to break up the gradient
    float glowNoise = fbm(uv * 4.0 + time * 0.05, 2) * 0.3 + 0.7;
    glow *= glowNoise;
    
    return vec3(glow);
}

// Particles
float getParticles(vec2 uv, float time)
{
    float particles = 0.0;
    
    for (int p = 0; p < 40; p++)
    {
        float seed = float(p);
        float depth = hash(vec2(seed, seed + 10.0));
        
        float speedMult = 0.5 + depth * 0.5;
        vec2 pPos = vec2(
            fract(sin(seed * 43.0) * 1000.0 + time * 0.05 * speedMult),
            fract(cos(seed * 37.0) * 1000.0 + time * 0.08 * speedMult + seed * 0.1)
        );
        
        pPos.x += sin(time * 2.0 + seed) * 0.02 * depth;
        
        float size = (0.0015 + hash(vec2(seed + 5.0, seed)) * 0.0025) * (0.6 + depth * 0.4);
        
        float dist = length(uv - pPos);
        float particle = smoothstep(size, 0.0, dist);
        
        particles += particle * (0.4 + depth * 0.6);
    }
    
    return particles;
}

// Bubbles
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

void main()
{
    vec2 uv = uv0;
    
    // === VERY STRONG WAVE DISTORTION ===
    // Layer 1: Large slow waves
    float bigWave1 = sin(uv.x * 20.0 + timeSeconds * 0.4) * cos(uv.y * 15.0 - timeSeconds * 0.3);
    float bigWave2 = cos(uv.x * 25.0 - timeSeconds * 0.35) * sin(uv.y * 20.0 + timeSeconds * 0.38);
    
    // Layer 2: Medium waves
    float medWave1 = sin(uv.x * 35.0 + timeSeconds * 0.6) * cos(uv.y * 30.0);
    float medWave2 = cos(uv.x * 40.0) * sin(uv.y * 35.0 - timeSeconds * 0.55);
    
    // Layer 3: Small fast waves
    float smallWave = sin(uv.x * 50.0 + uv.y * 45.0 + timeSeconds * 0.8);
    
    // Layer 4: Organic turbulence
    float turbulence = fbm(uv * 10.0 + vec2(timeSeconds * 0.12, -timeSeconds * 0.1), 3) - 0.5;
    
    // Combine all wave layers
    vec2 distortionVec = vec2(
        bigWave1 * 0.35 + medWave1 * 0.25 + smallWave * 0.15 + turbulence * 0.25,
        bigWave2 * 0.35 + medWave2 * 0.25 + smallWave * 0.15 + turbulence * 0.25
    ) * distortion * underwaterFactor;
    
    // === CHROMATIC ABERRATION ===
    vec2 aberrationDir = normalize(uv - 0.5);
    vec2 aberration = aberrationDir * chromaticAberration * underwaterFactor;
    
    float r = texture(rt0, uv + distortionVec + aberration).r;
    float g = texture(rt0, uv + distortionVec).g;
    float b = texture(rt0, uv + distortionVec - aberration).b;
    vec3 col = vec3(r, g, b);
    
    // === IMPROVED FAKE FOG (NO DEPTH BUFFER) - BALANCED VERSION ===
    
    // Calculate scene brightness/luminance
    float brightness = dot(col, vec3(0.299, 0.587, 0.114));
    
    // Brightness-based fog (REDUCED - only affects darker/distant pixels)
    float brightnessFog = pow(1.0 - clamp(brightness * 0.8, 0.0, 1.0), 3.5);
    
    // Radial distance from screen center
    vec2 centerOffset = uv - 0.5;
    float radialDist = length(centerOffset);
    float radialFog = pow(radialDist * 1.4, 4.0);
    
    // Vertical gradient (bottom = deeper = more fog) - REDUCED
    float verticalFog = pow(1.0 - uv.y, 4.5) * 0.6;
    
    // Animated volumetric fog layers - SUBTLE
    vec2 fogFlow1 = uv * 2.5 + vec2(timeSeconds * 0.018, -timeSeconds * 0.022);
    vec2 fogFlow2 = uv * 4.5 - vec2(timeSeconds * 0.015, timeSeconds * 0.025);
    vec2 fogFlow3 = uv * 7.0 + vec2(-timeSeconds * 0.012, timeSeconds * 0.018);
    
    float volumeFog = 
        fbm(fogFlow1, 4) * 0.2 +
        fbm(fogFlow2, 3) * 0.15 +
        fbm(fogFlow3, 2) * 0.1;
    
    // Edge darkness detector - REDUCED
    float edgeDarkness = 1.0 - clamp(brightness * 1.5, 0.0, 1.0);
    float edgeFogBoost = pow(edgeDarkness, 2.0) * radialFog * 0.5;
    
    // === COMBINE ALL FOG LAYERS - MUCH MORE CONSERVATIVE ===
    float totalFog = clamp(
        brightnessFog * 0.35 +
        radialFog * 0.15 +
        verticalFog * 0.25 +
        volumeFog * 0.15 +
        edgeFogBoost * 0.2,
        0.0, 1.0
    );
    
    // GENTLER fog buildup (higher power = slower fog increase)
    totalFog = pow(totalFog, 1.2);
    
    // Multi-layer fog colors for depth perception - LESS SATURATED
    vec3 nearFogColor = waterTint * 1.3;
    vec3 midFogColor = deepWaterTint * 1.4;
    vec3 farFogColor = deepWaterTint * 1.0;
    
    // Lerp between fog layers based on fog amount
    vec3 fogColor = mix(nearFogColor, midFogColor, clamp(totalFog * 1.2, 0.0, 1.0));
    fogColor = mix(fogColor, farFogColor, clamp(totalFog * totalFog * 1.5, 0.0, 1.0));
    
    // Apply fog with MUCH GENTLER strength
    float finalFogAmount = totalFog * fogStrength * underwaterFactor;
    col = mix(col, fogColor, clamp(finalFogAmount * 0.8, 0.0, 1.0));
    
    // === DARKEN DISTANT AREAS - GENTLER ===
    float distanceDarkening = pow(totalFog, 2.5);
    col *= mix(1.0, 0.5, distanceDarkening * underwaterFactor);
    
    // === SIMPLE DIRECTIONAL LIGHT GLOW (replaces god rays) ===
    vec3 lightGlow = getDirectionalGlow(uv, sunScreenPos, timeSeconds);
    col += lightGlow * lightGlowStrength * underwaterFactor *
           vec3(0.4, 0.65, 0.85);
    
    // === PARTICLES ===
    float particles = getParticles(uv, timeSeconds);
    col += particles * particleStrength * underwaterFactor * 
           vec3(0.55, 0.68, 0.78);
    
    // === BUBBLES ===
    float bubbles = getBubbles(uv, timeSeconds);
    col += bubbles * bubbleStrength * underwaterFactor * 
           vec3(0.7, 0.88, 1.0);
    
    // === EDGE DARKENING ===
    float edgeDarkening = pow(radialDist * 1.5, 2.5);
    float edgeTint = clamp(edgeDarkening * 0.7 + totalFog * 0.3, 0.0, 1.0);
    col = mix(col, waterTint * 1.4, edgeTint * underwaterFactor);
    
    // === DEPTH GRADIENT ===
    float depthGradient = pow(1.0 - uv.y, 2.5);
    col = mix(col, deepWaterTint * 1.3, depthGradient * 0.4 * underwaterFactor);
    
    // === COLOR GRADING WITH DISTANCE ===
    // Base color grading
    col.r *= mix(1.0, 0.68, underwaterFactor);
    col.g *= mix(1.0, 0.9, underwaterFactor);
    col.b *= mix(1.0, 1.25, underwaterFactor);
    
    // Additional color shift with distance - MUCH GENTLER
    col.r *= mix(1.0, 0.65, totalFog * underwaterFactor);
    col.g *= mix(1.0, 0.8, totalFog * underwaterFactor);
    col.b *= mix(1.0, 1.05, totalFog * underwaterFactor);
    
    // === CONTRAST ===
    col = (col - 0.5) * mix(1.0, contrast, underwaterFactor) + 0.5;
    
    // === SATURATION ===
    col = applySaturation(col, mix(1.0, saturation, underwaterFactor));
    
    // === VIGNETTE ===
    vec2 vignetteUV = (uv - 0.5) * 1.9;
    float vignetteAmount = pow(clamp(1.0 - dot(vignetteUV, vignetteUV) * 0.35, 0.0, 1.0), vignette);
    col *= mix(1.0, vignetteAmount, underwaterFactor);
    
    // === TURBIDITY NOISE ===
    float turbNoise = fbm(uv * 7.0 + timeSeconds * 0.022, 3) * 0.045;
    col += turbNoise * underwaterFactor * vec3(0.32, 0.42, 0.52);
    
    fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}