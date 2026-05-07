//-------------------------------
// Volumetric Light — Pass 3: Blend
//-------------------------------

Texture2D<float4> RT          : register(t0);
Texture2D<float4> GodRays     : register(t1);
SamplerState      samplerState : register(s0);

struct PS_INPUT
{
    float2 uv0 : TEXCOORD0;
};

float4 main(
    PS_INPUT inPs,
    uniform float  godRayStrength,
    uniform float3 tint
) : SV_Target
{
    float3 scene = RT.Sample( samplerState, inPs.uv0 ).rgb;
    float3 rays  = GodRays.Sample( samplerState, inPs.uv0 ).rgb;

    return float4( scene + rays * tint * godRayStrength, 1.0 );
}
