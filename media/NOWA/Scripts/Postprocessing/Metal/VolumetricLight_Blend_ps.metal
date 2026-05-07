//-------------------------------
// Volumetric Light — Pass 3: Blend
//-------------------------------

#include <metal_stdlib>
using namespace metal;

struct PS_INPUT
{
    float2 uv0;
};

struct Params
{
    float  godRayStrength;
    // Metal pads float3 to float4 — use float4 and ignore .w
    float4 tint;
};

fragment float4 main_metal(
    PS_INPUT             inPs        [[stage_in]],
    texture2d<float>     RT          [[texture(0)]],
    texture2d<float>     GodRays     [[texture(1)]],
    sampler              samplerState [[sampler(0)]],
    constant Params     &params      [[buffer(PARAMETER_SLOT)]]
)
{
    float3 scene = RT.sample( samplerState, inPs.uv0 ).rgb;
    float3 rays  = GodRays.sample( samplerState, inPs.uv0 ).rgb;

    float3 result = scene + rays * params.tint.xyz * params.godRayStrength;
    return float4( result.x, result.y, result.z, 1.0 );
}
