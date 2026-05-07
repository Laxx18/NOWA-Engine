//-------------------------------
// Volumetric Light — Pass 1: Sun Mask
//-------------------------------

Texture2D<float4> RT          : register(t0);
SamplerState      samplerState : register(s0);

struct PS_INPUT
{
    float2 uv0 : TEXCOORD0;
};

float4 main(
    PS_INPUT inPs,
    uniform float2 sunScreenPos,
    uniform float  sunThreshold,
    uniform float  sunRadius
) : SV_Target
{
    float3 color = RT.Sample( samplerState, inPs.uv0 ).rgb;
    float  lum   = dot( color, float3( 0.2126, 0.7152, 0.0722 ) );

    float brightMask = step( sunThreshold, lum );

    float sunDist   = length( inPs.uv0 - sunScreenPos );
    float radialMask = 1.0 - smoothstep( 0.0, sunRadius, sunDist );

    float mask = brightMask * (0.4 + radialMask * 0.6);

    return float4( color * mask, 1.0 );
}
