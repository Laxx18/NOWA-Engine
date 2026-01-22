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

// Simplex-like noise
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

// Fractal Brownian Motion for organic-looking fog/turbidity
static float fbm(float2 p)
{
    float value = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    
    for (int i = 0; i < 4; i++)
    {
        value += amplitude * noise(p * frequency);
        frequency *= 2.0f;
        amplitude *= 0.5f;
    }
    
    return value;
}

// Subtle caustics - less prominent, more realistic
static float getCaustics(float2 uv, float time)
{
    // Larger scale, slower movement
    float2 uv1 = uv * 3.0f + float2(time * 0.03f, time * 0.04f);
    float2 uv2 = uv * 3.5f - float2(time * 0.035f, time * 0.038f);
    
    // Softer caustics pattern
    float c1 = 0.5f + 0.5f * sin(uv1.x * 6.0f + sin(uv1.y * 4.0f));
    float c2 = 0.5f + 0.5f * sin(uv2.x * 5.5f + cos(uv2.y * 4.5f));
    
    float caustics = c1 * c2;
    caustics = pow(caustics, 2.5f); // Moderate sharpening
    
    // Fade caustics near edges
    float2 edgeDist = abs(uv - 0.5f) * 2.0f;
    float edgeFade = 1.0f - saturate(dot(edgeDist, edgeDist) * 0.5f);
    
    return caustics * edgeFade;
}

// Generate floating particles/sediment
static float getParticles(float2 uv, float time)
{
    float particles = 0.0f;
    
    // Fewer, subtler particles
    for (int p = 0; p < 15; p++)
    {
        float seed = float(p);
        
        // Particle position with slow drift
        float2 pPos = float2(
            frac(sin(seed * 43.0f) * 1000.0f + time * 0.03f),
            frac(cos(seed * 37.0f) * 1000.0f + time * 0.05f + seed * 0.1f)
        );
        
        // Slight horizontal wobble
        pPos.x += sin(time * 1.5f + seed) * 0.01f;
        
        // Smaller particles
        float size = 0.0015f + hash(float2(seed, seed)) * 0.0015f;
        
        float dist = length(uv - pPos);
        float particle = smoothstep(size, 0.0f, dist);
        
        // Vary opacity
        float opacity = 0.3f + hash(float2(seed + 5.0f, seed)) * 0.4f;
        particles += particle * opacity;
    }
    
    return particles;
}

// Enhanced fog with better distance simulation
static float getDistanceFog(float2 uv, float time)
{
    // Distance from center (simulates looking further away)
    float2 centerOffset = uv - 0.5f;
    float distFromCenter = length(centerOffset);
    
    // Vertical gradient (deeper = more fog)
    float verticalFog = pow(1.0f - uv.y, 1.5f) * 0.6f;
    
    // Animated turbidity using noise
    float2 noiseCoord = uv * 2.0f + float2(time * 0.015f, time * 0.02f);
    float turbidity = fbm(noiseCoord) * 0.3f + 0.7f;
    
    // Combine factors - stronger overall fog
    float fog = (pow(distFromCenter, 1.2f) * 0.5f + verticalFog) * turbidity;
    
    return saturate(fog);
}

// Generate rising bubbles
static float getBubbles(float2 uv, float time)
{
    float bubbles = 0.0f;
    
    // Fewer bubbles
    for (int b = 0; b < 8; b++)
    {
        float seed = float(b);
        float speed = 0.08f + hash(float2(seed, seed + 1.0f)) * 0.1f;
        
        // Bubble rises and wraps around
        float2 bPos = float2(
            frac(sin(seed * 23.0f) * 1000.0f),
            frac(time * speed + seed * 0.1f)
        );
        
        // Horizontal wobble
        bPos.x += sin(time * 2.5f + seed * 6.28f) * 0.015f;
        
        float size = 0.003f + hash(float2(seed + 2.0f, seed)) * 0.002f;
        float dist = length(uv - bPos);
        
        // Bubble with specular highlight
        float bubble = smoothstep(size, 0.0f, dist);
        float highlight = smoothstep(size * 0.4f, 0.0f, 
                                     length(uv - bPos + float2(size * 0.3f, -size * 0.3f)));
        
        bubbles += bubble * 0.3f + highlight * 0.6f;
    }
    
    return bubbles;
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
    uniform float causticsStrength,
    uniform float godRaysStrength,
    uniform float particleStrength,
    uniform float bubbleStrength,
    uniform float chromaticAberration,
    uniform float turbidity
) : SV_Target0
{
    float2 uv = inPs.uv0;
    
    // === WAVE DISTORTION (Subtle) ===
    float n1 = sin((uv.x + timeSeconds * 0.15f) * 30.0f);
    float n2 = sin((uv.y + timeSeconds * 0.12f) * 28.0f);
    float n3 = noise(uv * 8.0f + timeSeconds * 0.08f) * 0.3f;
    float n = (n1 * n2 + n3) * 0.5f;
    
    float2 distortionVec = float2(n, -n) * (distortion * 0.5f * underwaterFactor);
    
    // === SUBTLE CHROMATIC ABERRATION ===
    float2 aberration = float2(chromaticAberration * 0.5f, chromaticAberration * 0.5f) * underwaterFactor;
    float r = rt0.Sample(samp0, uv + distortionVec + aberration * 0.3f).r;
    float g = rt0.Sample(samp0, uv + distortionVec).g;
    float b = rt0.Sample(samp0, uv + distortionVec - aberration * 0.3f).b;
    float3 col = float3(r, g, b);
    
    // Store brightness for later use
    float brightness = dot(col, float3(0.299f, 0.587f, 0.114f));
    
    // === DISTANCE FOG (Primary visibility reducer) ===
    float distanceFog = getDistanceFog(uv, timeSeconds);
    float fogFactor = lerp(0.2f, 1.0f, 1.0f - distanceFog * fogStrength);
    
    // Apply fog with water color - much stronger
    float3 fogColor = lerp(waterTint, deepWaterTint, distanceFog * 0.5f);
    col = lerp(fogColor, col, fogFactor);
    
    // === SUBTLE CAUSTICS (only in brighter areas) ===
    float caustics = getCaustics(uv, timeSeconds);
    // Only apply caustics where there's already some light
    float causticsIntensity = saturate(brightness * 2.0f);
    col += caustics * causticsStrength * underwaterFactor * 
           float3(0.4f, 0.6f, 0.8f) * causticsIntensity * 0.3f;
    
    // === FLOATING PARTICLES ===
    float particles = getParticles(uv, timeSeconds);
    col += particles * particleStrength * underwaterFactor * 
           float3(0.55f, 0.65f, 0.75f) * 0.5f;
    
    // === BUBBLES ===
    float bubbles = getBubbles(uv, timeSeconds);
    col += bubbles * bubbleStrength * underwaterFactor * 
           float3(0.7f, 0.8f, 0.9f) * 0.4f;
    
    // === WATER ABSORPTION/TINT (Stronger) ===
    float edgeFactor = pow(length(uv - 0.5f) * 2.0f, 1.2f);
    float tintStrength = lerp(0.25f, 0.55f, edgeFactor);
    col = lerp(col, waterTint, tintStrength * underwaterFactor);
    
    // === CONTRAST ADJUSTMENT (Reduce contrast underwater) ===
    float underwaterContrast = lerp(1.0f, contrast * 0.85f, underwaterFactor);
    col = (col - 0.5f) * underwaterContrast + 0.5f;
    
    // === SATURATION ADJUSTMENT (Reduce saturation) ===
    float underwaterSaturation = lerp(1.0f, saturation * 0.7f, underwaterFactor);
    col = applySaturation(col, underwaterSaturation);
    
    // === VIGNETTE (Stronger) ===
    float2 d = abs(uv - 0.5f) * 2.0f;
    float v = saturate(1.0f - dot(d, d) * vignette * 1.5f);
    col *= lerp(1.0f, v, underwaterFactor);
    
    // === DEPTH GRADIENT (fake depth by darkening bottom) ===
    float depthGradient = pow(uv.y, 1.5f);
    col = lerp(col, deepWaterTint * 0.5f, (1.0f - depthGradient) * 0.4f * underwaterFactor);
    
    // === ANIMATED VOLUMETRIC FOG OVERLAY ===
    float2 noiseUV = uv * 3.0f + float2(timeSeconds * 0.01f, timeSeconds * 0.015f);
    float volumetricFog = fbm(noiseUV);
    col = lerp(col, waterTint * 0.8f, volumetricFog * 0.15f * underwaterFactor);
    
    // === FINAL DARKENING (Reduce overall brightness) ===
    col *= lerp(1.0f, 0.7f, underwaterFactor);
    
    return float4(saturate(col), 1.0f);
}