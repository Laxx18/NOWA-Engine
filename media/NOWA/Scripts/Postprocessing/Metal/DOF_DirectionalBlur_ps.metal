//-------------------------------
// Depth of Field — Directional Blur (Metal)
//-------------------------------

#include <metal_stdlib>
using namespace metal;

struct PS_INPUT { float2 uv0; };

struct Params
{
    float2 blurDir;
    float  blurRadius;
    float  numSamples;
};

fragment float4 main_metal(
    PS_INPUT             inPs        [[stage_in]],
    texture2d<float>     RT          [[texture(0)]],
    sampler              samplerState [[sampler(0)]],
    constant Params     &params      [[buffer(PARAMETER_SLOT)]]
)
{
    float4 center = RT.sample( samplerState, inPs.uv0 );
    float  coc    = center.a;

    if ( coc < 0.005f )
        return center;

    float  stepSize = (params.blurRadius * coc) / params.numSamples;
    float2 stepVec  = params.blurDir * stepSize;

    float4 accumulated = float4(0.0);
    float  totalWeight = 0.0f;

    int iSamples = (int)params.numSamples;

    for ( int i = -iSamples; i <= iSamples; i++ )
    {
        float2 sampleUV = inPs.uv0 + stepVec * (float)i;
        float4 s        = RT.sample( samplerState, sampleUV );
        float  w        = max( s.a, coc * 0.5f );
        accumulated    += s * w;
        totalWeight    += w;
    }

    return accumulated / max( totalWeight, 0.0001f );
}
