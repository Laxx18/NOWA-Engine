//-------------------------------
// Depth Fog + Height Fog compositor effect
//-------------------------------

#include <metal_stdlib>
using namespace metal;

struct PS_INPUT
{
    float2 uv0;
};

struct Params
{
    float2   projectionParams;
    float    farClipDistance;
    float    _pad0;           // align to float4 boundary before matrix
    float4x4 invViewProjMatrix;
    float    depthFogDensity;
    float    depthFogStart;
    float    heightFogDensity;
    float    heightFogStart;
    float    heightFogEnd;
    float    fogSkyBlend;
    float    _pad1;
    float    _pad2;
    float4   fogColor;        // Metal pads float3 → use float4, ignore .w
};

static float getLinearDepth(float rawDepth, float2 projectionParams, float farClipDistance)
{
    float denom = rawDepth - projectionParams.x;
    if (fabs(denom) < 0.0001f)
        denom = 0.0001f;
    return (projectionParams.y / denom) * farClipDistance;
}

static float3 getWorldPos(float2 uv, float rawDepth, float4x4 invViewProjMatrix)
{
    // Metal uses column-major but treats clip Z in [0,1] on Apple Silicon
    float4 ndcPos  = float4(uv.x * 2.0f - 1.0f,
                            (1.0f - uv.y) * 2.0f - 1.0f,
                            rawDepth,
                            1.0f);
    float4 worldPos = invViewProjMatrix * ndcPos;
    return worldPos.xyz / worldPos.w;
}

fragment float4 main_metal(
    PS_INPUT             inPs        [[stage_in]],
    texture2d<float>     RT          [[texture(0)]],
    texture2d<float>     DepthTex    [[texture(1)]],
    sampler              samplerBilinear [[sampler(0)]],
    sampler              samplerPoint    [[sampler(1)]],
    constant Params     &params      [[buffer(PARAMETER_SLOT)]]
)
{
    float3 sceneColor = RT.sample(samplerBilinear, inPs.uv0).rgb;
    float  rawDepth   = DepthTex.sample(samplerPoint, inPs.uv0).r;

    float linearDepth = getLinearDepth(rawDepth, params.projectionParams, params.farClipDistance);

    // Depth fog
    float depthFogFactor = 0.0f;
    if (params.depthFogDensity > 0.0f)
    {
        float effectiveDepth = max(0.0f, linearDepth - params.depthFogStart);
        depthFogFactor = 1.0f - exp(-params.depthFogDensity * effectiveDepth);
        depthFogFactor = saturate(depthFogFactor);
    }

    // Height fog
    float heightFogFactor = 0.0f;
    if (params.heightFogDensity > 0.0f)
    {
        float3 worldPos = getWorldPos(inPs.uv0, rawDepth, params.invViewProjMatrix);
        float range     = max(params.heightFogEnd - params.heightFogStart, 0.001f);
        float normalizedHeight = saturate((worldPos.y - params.heightFogStart) / range);
        float heightFade  = 1.0f - normalizedHeight;
        heightFogFactor   = 1.0f - exp(-params.heightFogDensity * heightFade * linearDepth * 0.1f);
        heightFogFactor   = saturate(heightFogFactor);
    }

    float totalFog    = saturate(max(depthFogFactor, heightFogFactor));
    float skySuppress = (rawDepth >= 0.9999f) ? (1.0f - params.fogSkyBlend) : 1.0f;
    totalFog         *= skySuppress;

    float3 result = mix(sceneColor, params.fogColor.xyz, totalFog);
    return float4(result.x, result.y, result.z, 1.0f);
}
