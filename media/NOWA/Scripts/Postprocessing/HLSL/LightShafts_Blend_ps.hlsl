//-------------------------------
// Light Shafts — Pass 3: Blend
//-------------------------------

Texture2D<float4> RT        : register(t0);
Texture2D<float4> ShaftTex  : register(t1);
SamplerState      samplerState : register(s0);

struct PS_INPUT
{
    float2 uv0 : TEXCOORD0;
};

float4 main(
    PS_INPUT inPs,
    uniform float  shaftStrength,
    uniform float3 tint
) : SV_Target
{
    float3 scene  = RT.Sample(       samplerState, inPs.uv0 ).rgb;
    float3 shafts = ShaftTex.Sample( samplerState, inPs.uv0 ).rgb;

    return float4( scene + shafts * tint * shaftStrength, 1.0 );
}
