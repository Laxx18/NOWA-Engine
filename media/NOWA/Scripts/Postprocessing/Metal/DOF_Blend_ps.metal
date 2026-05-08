//-------------------------------
// Depth of Field — Blend Pass (Metal)
//-------------------------------

#include <metal_stdlib>
using namespace metal;

struct PS_INPUT { float2 uv0; };
struct Params { float blendStrength; float _pad0; float _pad1; float _pad2; };

fragment float4 main_metal(
    PS_INPUT             inPs        [[stage_in]],
    texture2d<float>     RT_Input    [[texture(0)]],
    texture2d<float>     BlurredRT   [[texture(1)]],
    texture2d<float>     CoCRT       [[texture(2)]],
    sampler              samplerState [[sampler(0)]],
    constant Params     &params      [[buffer(PARAMETER_SLOT)]]
)
{
    float3 sharp   = RT_Input.sample(  samplerState, inPs.uv0 ).rgb;
    float3 blurred = BlurredRT.sample( samplerState, inPs.uv0 ).rgb;
    float  coc     = CoCRT.sample(     samplerState, inPs.uv0 ).a;

    float blend  = saturate( coc * params.blendStrength );
    float3 result = mix( sharp, blurred, blend );
    return float4( result.x, result.y, result.z, 1.0f );
}
