//-------------------------------
// Depth of Field — Hexagonal Bokeh Combine (Metal)
//-------------------------------

#include <metal_stdlib>
using namespace metal;

struct PS_INPUT { float2 uv0; };

fragment float4 main_metal(
    PS_INPUT             inPs        [[stage_in]],
    texture2d<float>     RT0         [[texture(0)]],
    texture2d<float>     RT1         [[texture(1)]],
    texture2d<float>     RT2         [[texture(2)]],
    sampler              samplerState [[sampler(0)]]
)
{
    float4 s0 = RT0.sample( samplerState, inPs.uv0 );
    float4 s1 = RT1.sample( samplerState, inPs.uv0 );
    float4 s2 = RT2.sample( samplerState, inPs.uv0 );

    return (s0 + s1 + s2) * (1.0f / 3.0f);
}
