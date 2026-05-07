//-------------------------------
// Depth Fog + Height Fog compositor effect
//-------------------------------

Texture2D<float4> RT       : register(t0);  // scene colour
Texture2D<float>  DepthTex : register(t1);  // PFG_D32_FLOAT depth

SamplerState samplerBilinear : register(s0);
SamplerState samplerPoint    : register(s1);

struct PS_INPUT
{
    float2 uv0 : TEXCOORD0;
};

// Depth linearisation — identical to Underwater / SSAO
static float getLinearDepth(float rawDepth, float2 projectionParams, float farClipDistance)
{
    float denom = rawDepth - projectionParams.x;
    if (abs(denom) < 0.0001f)
        denom = 0.0001f;
    return (projectionParams.y / denom) * farClipDistance;
}

// World-space position reconstruction
static float3 getWorldPos(float2 uv, float rawDepth, float4x4 invViewProjMatrix)
{
    // DX NDC Z is already [0,1] — no remap needed
    float4 ndcPos  = float4(uv.x * 2.0f - 1.0f,
                            (1.0f - uv.y) * 2.0f - 1.0f,
                            rawDepth,
                            1.0f);
    float4 worldPos = mul(invViewProjMatrix, ndcPos);
    return worldPos.xyz / worldPos.w;
}

float4 main(
    PS_INPUT inPs,

    // Depth linearisation
    uniform float2   projectionParams,
    uniform float    farClipDistance,

    // World-space reconstruction
    uniform float4x4 invViewProjMatrix,

    // Depth fog
    uniform float depthFogDensity,
    uniform float depthFogStart,

    // Height fog
    uniform float heightFogDensity,
    uniform float heightFogStart,
    uniform float heightFogEnd,

    // Shared
    uniform float3 fogColor,
    uniform float  fogSkyBlend

) : SV_Target
{
    float3 sceneColor = RT.Sample(samplerBilinear, inPs.uv0).rgb;
    float  rawDepth   = DepthTex.Sample(samplerPoint, inPs.uv0).r;

    float linearDepth = getLinearDepth(rawDepth, projectionParams, farClipDistance);

    // Depth fog
    float depthFogFactor = 0.0f;
    if (depthFogDensity > 0.0f)
    {
        float effectiveDepth = max(0.0f, linearDepth - depthFogStart);
        depthFogFactor = 1.0f - exp(-depthFogDensity * effectiveDepth);
        depthFogFactor = saturate(depthFogFactor);
    }

    // Height fog
    float heightFogFactor = 0.0f;
    if (heightFogDensity > 0.0f)
    {
        float3 worldPos = getWorldPos(inPs.uv0, rawDepth, invViewProjMatrix);
        float  normalizedHeight = saturate(
            (worldPos.y - heightFogStart) / max(heightFogEnd - heightFogStart, 0.001f));
        float heightFade  = 1.0f - normalizedHeight;
        heightFogFactor   = 1.0f - exp(-heightFogDensity * heightFade * linearDepth * 0.1f);
        heightFogFactor   = saturate(heightFogFactor);
    }

    float totalFog   = saturate(max(depthFogFactor, heightFogFactor));
    float skySuppress = (rawDepth >= 0.9999f) ? (1.0f - fogSkyBlend) : 1.0f;
    totalFog         *= skySuppress;

    return float4(lerp(sceneColor, fogColor, totalFog), 1.0f);
}
