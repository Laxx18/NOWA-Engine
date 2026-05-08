//-------------------------------
// Light Shafts — Pass 1: Occlusion Mask
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
    float  occlusionDepthThreshold;
    float  sunRadius;
    float  brightnessThreshold;
    float  _pad0;
    float  _pad1;
    float  _pad2;
};

fragment float4 main_metal(
    PS_INPUT             inPs             [[stage_in]],
    texture2d<float>     RT               [[texture(0)]],
    texture2d<float>     DepthTex         [[texture(1)]],
    sampler              samplerBilinear  [[sampler(0)]],
    sampler              samplerPoint     [[sampler(1)]],
    constant Params     &params           [[buffer(PARAMETER_SLOT)]]
)
{
    float  rawDepth = DepthTex.sample( samplerPoint,    inPs.uv0 ).r;
    float3 color    = RT.sample(       samplerBilinear, inPs.uv0 ).rgb;

    float isSky    = step( params.occlusionDepthThreshold, rawDepth );
    float lum      = dot( color, float3( 0.2126, 0.7152, 0.0722 ) );
    float isBright = step( params.brightnessThreshold, lum );

    float sunDist    = length( inPs.uv0 - params.sunScreenPos );
    float radialMask = 1.0 - smoothstep( 0.0, params.sunRadius, sunDist );

    float emission = isSky * isBright * (0.3 + radialMask * 0.7);
    float3 result  = color * emission;

    return float4( result.x, result.y, result.z, 1.0 );
}
