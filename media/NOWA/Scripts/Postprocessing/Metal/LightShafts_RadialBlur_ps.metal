//-------------------------------
// Light Shafts — Pass 2: Radial Blur
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
    float  decay;
    float  density;
    float  weight;
    float  exposure;
    float  shaftSharpness;
    float  _pad;
};

constant int NUM_SAMPLES = 48;

fragment float4 main_metal(
    PS_INPUT             inPs        [[stage_in]],
    texture2d<float>     RT          [[texture(0)]],
    sampler              samplerState [[sampler(0)]],
    constant Params     &params      [[buffer(PARAMETER_SLOT)]]
)
{
    float2 uv       = inPs.uv0;
    float2 delta    = (params.sunScreenPos - uv) * (params.density / (float)NUM_SAMPLES);
    float2 sampleUV = uv;

    float3 accumulated       = float3( 0.0 );
    float  illuminationDecay = 1.0;

    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        sampleUV  += delta;
        float3 s   = RT.sample( samplerState, sampleUV ).rgb;
        accumulated       += s * illuminationDecay * params.weight;
        illuminationDecay *= params.decay;
    }

    accumulated *= params.exposure;
    float invSharp = 1.0 / max( params.shaftSharpness, 0.001f );
    accumulated = pow( clamp( accumulated, 0.0, 1.0 ), float3( invSharp ) );

    return float4( accumulated.x, accumulated.y, accumulated.z, 1.0 );
}
