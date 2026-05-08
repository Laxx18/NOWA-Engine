//-------------------------------
// Light Shafts — Pass 3: Blend
//-------------------------------

#include <metal_stdlib>
using namespace metal;

struct PS_INPUT
{
    float2 uv0;
};

struct Params
{
    float  shaftStrength;
    float  _pad0;
    float  _pad1;
    float  _pad2;
    float4 tint;  // Metal pads float3 → float4
};

fragment float4 main_metal(
    PS_INPUT             inPs        [[stage_in]],
    texture2d<float>     RT          [[texture(0)]],
    texture2d<float>     ShaftTex    [[texture(1)]],
    sampler              samplerState [[sampler(0)]],
    constant Params     &params      [[buffer(PARAMETER_SLOT)]]
)
{
    float3 scene  = RT.sample(       samplerState, inPs.uv0 ).rgb;
    float3 shafts = ShaftTex.sample( samplerState, inPs.uv0 ).rgb;

    float3 result = scene + shafts * params.tint.xyz * params.shaftStrength;
    return float4( result.x, result.y, result.z, 1.0 );
}
