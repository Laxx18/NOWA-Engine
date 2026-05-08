//-------------------------------
// Depth of Field — Hexagonal Bokeh Combine
//-------------------------------

Texture2D<float4> RT0         : register(t0);
Texture2D<float4> RT1         : register(t1);
Texture2D<float4> RT2         : register(t2);
SamplerState      samplerState : register(s0);

struct PS_INPUT { float2 uv0 : TEXCOORD0; };

float4 main( PS_INPUT inPs ) : SV_Target
{
    float4 s0 = RT0.Sample( samplerState, inPs.uv0 );
    float4 s1 = RT1.Sample( samplerState, inPs.uv0 );
    float4 s2 = RT2.Sample( samplerState, inPs.uv0 );

    return (s0 + s1 + s2) * (1.0f / 3.0f);
}
