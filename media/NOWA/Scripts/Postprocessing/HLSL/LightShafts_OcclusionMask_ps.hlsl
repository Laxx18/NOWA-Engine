//-------------------------------
// Light Shafts — Pass 1: Occlusion Mask
//-------------------------------

Texture2D<float4> RT       : register(t0);
Texture2D<float>  DepthTex : register(t1);

SamplerState samplerBilinear : register(s0);
SamplerState samplerPoint    : register(s1);

struct PS_INPUT
{
    float2 uv0 : TEXCOORD0;
};

float4 main(
    PS_INPUT inPs,
    uniform float2 sunScreenPos,
    uniform float  occlusionDepthThreshold,
    uniform float  sunRadius,
    uniform float  brightnessThreshold
) : SV_Target
{
    float  rawDepth = DepthTex.Sample( samplerPoint,    inPs.uv0 ).r;
    float3 color    = RT.Sample(       samplerBilinear, inPs.uv0 ).rgb;

    // Gate 1: geometry occlusion
    float isSky = step( rawDepth, occlusionDepthThreshold );

    // Gate 2: brightness
    float lum      = dot( color, float3( 0.2126, 0.7152, 0.0722 ) );
    float isBright = step( brightnessThreshold, lum );

    // Gate 3: radial proximity to sun
    float sunDist    = length( inPs.uv0 - sunScreenPos );
    float radialMask = 1.0 - smoothstep( 0.0, sunRadius, sunDist );

    float emission = isSky * isBright * (0.3 + radialMask * 0.7);

    return float4( color * emission, 1.0 );
}
