//-------------------------------
// Depth of Field — Blend Pass
//-------------------------------

Texture2D<float4> RT_Input  : register(t0);
Texture2D<float4> BlurredRT : register(t1);
Texture2D<float4> CoCRT     : register(t2);

SamplerState samplerState : register(s0);

struct PS_INPUT { float2 uv0 : TEXCOORD0; };

float4 main(
    PS_INPUT inPs,
    uniform float blendStrength
) : SV_Target
{
    float3 sharp   = RT_Input.Sample(  samplerState, inPs.uv0 ).rgb;
    float3 blurred = BlurredRT.Sample( samplerState, inPs.uv0 ).rgb;
    float  coc     = CoCRT.Sample(     samplerState, inPs.uv0 ).a;

    float blend = saturate( coc * blendStrength );
    return float4( lerp( sharp, blurred, blend ), 1.0f );
}
