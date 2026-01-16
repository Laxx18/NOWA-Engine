Texture2D<float4> rt0 : register(t0);
SamplerState samp0    : register(s0);

struct PS_INPUT
{
    float2 uv0 : TEXCOORD0;
};

// Apply saturation adjustment
static float3 applySaturation(float3 c, float s)
{
    float l = dot(c, float3(0.2126f, 0.7152f, 0.0722f));
    return lerp(l.xxx, c, s);
}

// Hash function for procedural noise
static float hash(float2 p)
{
    return frac(sin(dot(p, float2(127.1f, 311.7f))) * 43758.5453f);
}

// Improved noise function
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

// Fractal Brownian Motion
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

// Simple directional light glow instead of god rays
static float3 getDirectionalGlow(float2 uv, float2 lightPos, float time)
{
    // Distance from light source
    float2 toLight = lightPos - uv;
    float dist = length(toLight);
    
    // Simple radial gradient
    float glow = pow(saturate(1.0f - dist * 0.8f), 3.0f);
    
    // Add subtle animation
    float pulse = sin(time * 0.3f) * 0.1f + 0.9f;
    glow *= pulse;
    
    // Add some noise to break up the gradient
    float glowNoise = fbm(uv * 4.0f + time * 0.05f, 2) * 0.3f + 0.7f;
    glow *= glowNoise;
    
    return float3(glow, glow, glow);
}

// Particles
static float getParticles(float2 uv, float time)
{
    float particles = 0.0f;
    
    for (int p = 0; p < 40; p++)
    {
        float seed = float(p);
        float depth = hash(float2(seed, seed + 10.0f));
        
        float speedMult = 0.5f + depth * 0.5f;
        float2 pPos = float2(
            frac(sin(seed * 43.0f) * 1000.0f + time * 0.05f * speedMult),
            frac(cos(seed * 37.0f) * 1000.0f + time * 0.08f * speedMult + seed * 0.1f)
        );
        
        pPos.x += sin(time * 2.0f + seed) * 0.02f * depth;
        
        float size = (0.0015f + hash(float2(seed + 5.0f, seed)) * 0.0025f) * (0.6f + depth * 0.4f);
        
        float dist = length(uv - pPos);
        float particle = smoothstep(size, 0.0f, dist);
        
        particles += particle * (0.4f + depth * 0.6f);
    }
    
    return particles;
}

// Bubbles
static float getBubbles(float2 uv, float time)
{
    float bubbles = 0.0f;
    
    for (int b = 0; b < 20; b++)
    {
        float seed = float(b);
        float speed = 0.12f + hash(float2(seed, seed + 1.0f)) * 0.18f;
        
        float yOffset = frac(time * speed + seed * 0.08f);
        float2 bPos = float2(
            frac(sin(seed * 23.0f) * 1000.0f),
            yOffset
        );
        
        bPos.x += sin(time * 3.0f + seed * 6.28f) * 0.035f;
        bPos.x += sin(time * 5.5f + seed * 3.14f) * 0.022f;
        
        float size = 0.004f + hash(float2(seed + 2.0f, seed)) * 0.005f;
        float dist = length(uv - bPos);
        
        float bubble = smoothstep(size, size * 0.3f, dist) - smoothstep(size * 1.2f, size, dist);
        float highlight = smoothstep(size * 0.4f, 0.0f, 
                                     length(uv - bPos + float2(size * 0.4f, -size * 0.4f)));
        
        bubbles += bubble * 0.7f + highlight * 1.8f;
    }
    
    return saturate(bubbles);
}

float4 main(
    PS_INPUT inPs,
    uniform float underwaterFactor,
    uniform float timeSeconds,
    uniform float3 waterTint,
    uniform float3 deepWaterTint,
    uniform float distortion,
    uniform float contrast,
    uniform float saturation,
    uniform float vignette,
    uniform float2 sunScreenPos,
    uniform float fogStrength,
    uniform float lightGlowStrength,
    uniform float particleStrength,
    uniform float bubbleStrength,
    uniform float chromaticAberration,
    uniform float turbidity
) : SV_Target0
{
    float2 uv = inPs.uv0;
    
    // === VERY STRONG WAVE DISTORTION ===
    // Layer 1: Large slow waves
    float bigWave1 = sin(uv.x * 20.0f + timeSeconds * 0.4f) * cos(uv.y * 15.0f - timeSeconds * 0.3f);
    float bigWave2 = cos(uv.x * 25.0f - timeSeconds * 0.35f) * sin(uv.y * 20.0f + timeSeconds * 0.38f);
    
    // Layer 2: Medium waves
    float medWave1 = sin(uv.x * 35.0f + timeSeconds * 0.6f) * cos(uv.y * 30.0f);
    float medWave2 = cos(uv.x * 40.0f) * sin(uv.y * 35.0f - timeSeconds * 0.55f);
    
    // Layer 3: Small fast waves
    float smallWave = sin(uv.x * 50.0f + uv.y * 45.0f + timeSeconds * 0.8f);
    
    // Layer 4: Organic turbulence
    float turbulence = fbm(uv * 10.0f + float2(timeSeconds * 0.12f, -timeSeconds * 0.1f), 3) - 0.5f;
    
    // Combine all wave layers
    float2 distortionVec = float2(
        bigWave1 * 0.35f + medWave1 * 0.25f + smallWave * 0.15f + turbulence * 0.25f,
        bigWave2 * 0.35f + medWave2 * 0.25f + smallWave * 0.15f + turbulence * 0.25f
    ) * distortion * underwaterFactor;
    
    // === CHROMATIC ABERRATION ===
    float2 aberrationDir = normalize(uv - 0.5f);
    float2 aberration = aberrationDir * chromaticAberration * underwaterFactor;
    
    float r = rt0.Sample(samp0, uv + distortionVec + aberration).r;
    float g = rt0.Sample(samp0, uv + distortionVec).g;
    float b = rt0.Sample(samp0, uv + distortionVec - aberration).b;
    float3 col = float3(r, g, b);
    
    // === IMPROVED FAKE FOG (NO DEPTH BUFFER) - BALANCED VERSION ===

	// Calculate scene brightness/luminance
	float brightness = dot(col, float3(0.299f, 0.587f, 0.114f));

	// Brightness-based fog (REDUCED - only affects darker/distant pixels)
	float brightnessFog = pow(1.0f - saturate(brightness * 0.8f), 3.5f); // Increased power, reduced multiplier

	// Radial distance from screen center
	float2 centerOffset = uv - 0.5f;
	float radialDist = length(centerOffset);
	float radialFog = pow(radialDist * 1.4f, 4.0f); // Increased power for sharper falloff

	// Vertical gradient (bottom = deeper = more fog) - REDUCED
	float verticalFog = pow(1.0f - uv.y, 4.5f) * 0.6f; // Higher power, lower multiplier

	// Animated volumetric fog layers - SUBTLE
	float2 fogFlow1 = uv * 2.5f + float2(timeSeconds * 0.018f, -timeSeconds * 0.022f);
	float2 fogFlow2 = uv * 4.5f - float2(timeSeconds * 0.015f, timeSeconds * 0.025f);
	float2 fogFlow3 = uv * 7.0f + float2(-timeSeconds * 0.012f, timeSeconds * 0.018f);

	float volumeFog = 
		fbm(fogFlow1, 4) * 0.2f +  // REDUCED from 0.4
		fbm(fogFlow2, 3) * 0.15f + // REDUCED from 0.35
		fbm(fogFlow3, 2) * 0.1f;   // REDUCED from 0.25

	// Edge darkness detector - REDUCED
	float edgeDarkness = 1.0f - saturate(brightness * 1.5f); // Reduced sensitivity
	float edgeFogBoost = pow(edgeDarkness, 2.0f) * radialFog * 0.5f; // Added 0.5x multiplier

	// === COMBINE ALL FOG LAYERS - MUCH MORE CONSERVATIVE ===
	float totalFog = saturate(
		brightnessFog * 0.35f +      // REDUCED from 0.55
		radialFog * 0.15f +           // REDUCED from 0.25
		verticalFog * 0.25f +         // REDUCED from 0.4
		volumeFog * 0.15f +           // REDUCED from 0.3
		edgeFogBoost * 0.2f           // REDUCED from 0.35
	);

	// GENTLER fog buildup (higher power = slower fog increase)
	totalFog = pow(totalFog, 1.2f); // CHANGED from 0.7 (was making fog too strong)

	// Multi-layer fog colors for depth perception - LESS SATURATED
	float3 nearFogColor = waterTint * 1.3f;          // REDUCED from 2.0
	float3 midFogColor = deepWaterTint * 1.4f;       // REDUCED from 1.8
	float3 farFogColor = deepWaterTint * 1.0f;       // REDUCED from 1.2

	// Lerp between fog layers based on fog amount
	float3 fogColor = lerp(nearFogColor, midFogColor, saturate(totalFog * 1.2f)); // REDUCED from 1.5
	fogColor = lerp(fogColor, farFogColor, saturate(totalFog * totalFog * 1.5f)); // REDUCED from 2.0

	// Apply fog with MUCH GENTLER strength
	float finalFogAmount = totalFog * fogStrength * underwaterFactor;
	col = lerp(col, fogColor, saturate(finalFogAmount * 0.8f)); // REDUCED from 1.5x to 0.8x

	// === DARKEN DISTANT AREAS - GENTLER ===
	float distanceDarkening = pow(totalFog, 2.5f); // INCREASED power for gentler darkening
	col *= lerp(1.0f, 0.5f, distanceDarkening * underwaterFactor); // REDUCED from 0.3 to 0.5 (less dark)
    
    // === SIMPLE DIRECTIONAL LIGHT GLOW (replaces god rays) ===
    float3 lightGlow = getDirectionalGlow(uv, sunScreenPos, timeSeconds);
    col += lightGlow * lightGlowStrength * underwaterFactor *
           float3(0.4f, 0.65f, 0.85f);
    
    // === PARTICLES ===
    float particles = getParticles(uv, timeSeconds);
    col += particles * particleStrength * underwaterFactor * 
           float3(0.55f, 0.68f, 0.78f);
    
    // === BUBBLES ===
    float bubbles = getBubbles(uv, timeSeconds);
    col += bubbles * bubbleStrength * underwaterFactor * 
           float3(0.7f, 0.88f, 1.0f);
    
    // === EDGE DARKENING ===
    float edgeDarkening = pow(radialDist * 1.5f, 2.5f);
    float edgeTint = saturate(edgeDarkening * 0.7f + totalFog * 0.3f);
    col = lerp(col, waterTint * 1.4f, edgeTint * underwaterFactor);
    
    // === DEPTH GRADIENT ===
    float depthGradient = pow(1.0f - uv.y, 2.5f);
    col = lerp(col, deepWaterTint * 1.3f, depthGradient * 0.4f * underwaterFactor);
    
    // === COLOR GRADING WITH DISTANCE ===
	// Base color grading
	col.r *= lerp(1.0f, 0.68f, underwaterFactor);
	col.g *= lerp(1.0f, 0.9f, underwaterFactor);
	col.b *= lerp(1.0f, 1.25f, underwaterFactor);

	// Additional color shift with distance - MUCH GENTLER
	col.r *= lerp(1.0f, 0.65f, totalFog * underwaterFactor); // REDUCED from 0.4
	col.g *= lerp(1.0f, 0.8f, totalFog * underwaterFactor);  // REDUCED from 0.65
	col.b *= lerp(1.0f, 1.05f, totalFog * underwaterFactor); // REDUCED from 1.1
    
    // === CONTRAST ===
    col = (col - 0.5f) * lerp(1.0f, contrast, underwaterFactor) + 0.5f;
    
    // === SATURATION ===
    col = applySaturation(col, lerp(1.0f, saturation, underwaterFactor));
    
    // === VIGNETTE ===
    float2 vignetteUV = (uv - 0.5f) * 1.9f;
    float vignetteAmount = pow(saturate(1.0f - dot(vignetteUV, vignetteUV) * 0.35f), vignette);
    col *= lerp(1.0f, vignetteAmount, underwaterFactor);
    
    // === TURBIDITY NOISE ===
    float turbNoise = fbm(uv * 7.0f + timeSeconds * 0.022f, 3) * 0.045f;
    col += turbNoise * underwaterFactor * float3(0.32f, 0.42f, 0.52f);
    
    return float4(saturate(col), 1.0f);
}