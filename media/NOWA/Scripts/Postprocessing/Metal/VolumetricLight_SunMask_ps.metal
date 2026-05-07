//-------------------------------
// Volumetric Light — Pass 1: Sun Mask
//-------------------------------

#include <metal_stdlib>
using namespace metal;

struct PS_INPUT
{
    float2 uv0;
};

struct Params
{
    float2 sunScreenPos;
    float  sunThreshold;
    float  sunRadius;
};

fragment float4 main_metal(
    PS_INPUT             inPs        [[stage_in]],
    texture2d<float>     RT          [[texture(0)]],
    sampler              samplerState [[sampler(0)]],
    constant Params     &params      [[buffer(PARAMETER_SLOT)]]
)
{
    float3 color = RT.sample( samplerState, inPs.uv0 ).rgb;
    float  lum   = dot( color, float3( 0.2126, 0.7152, 0.0722 ) );

    float brightMask = step( params.sunThreshold, lum );

    float sunDist    = length( inPs.uv0 - params.sunScreenPos );
    float radialMask = 1.0 - smoothstep( 0.0, params.sunRadius, sunDist );

    float mask = brightMask * (0.4 + radialMask * 0.6);

    return float4( color * mask, 1.0 );
}
