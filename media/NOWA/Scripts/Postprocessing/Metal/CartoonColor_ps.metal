//-------------------------------
// Cartoon: Cel-shade + edge composite pass
//-------------------------------

#include <metal_stdlib>
using namespace metal;

struct PS_INPUT
{
    float2 uv0;
};

struct Params
{
    float numBands;
    float saturation;
    float edgeDarkness;
};

float3 adjustSaturation( float3 color, float sat )
{
    float grey = dot( color, float3( 0.299, 0.587, 0.114 ) );
    return mix( float3( grey ), color, sat );
}

fragment float4 main_metal(
    PS_INPUT             inPs        [[stage_in]],
    texture2d<float>     RT          [[texture(0)]],
    texture2d<float>     EdgeTex     [[texture(1)]],
    sampler              samplerState [[sampler(0)]],
    constant Params     &params      [[buffer(PARAMETER_SLOT)]]
)
{
    float3 color = RT.sample( samplerState, inPs.uv0 ).rgb;
    float  edge  = EdgeTex.sample( samplerState, inPs.uv0 ).r;

    // Quantise into discrete cel bands
    color = floor( color * params.numBands + 0.5 ) / params.numBands;

    // Saturation boost
    color = adjustSaturation( color, params.saturation );

    // Edge darkening
    color *= mix( 1.0, params.edgeDarkness, edge );

    return float4( color.x, color.y, color.z, 1.0 );
}
