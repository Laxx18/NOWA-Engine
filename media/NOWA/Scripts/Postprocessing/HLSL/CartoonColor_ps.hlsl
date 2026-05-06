//-------------------------------
// Cartoon: Cel-shade + edge composite pass
//-------------------------------

Texture2D RT             : register(t0);
Texture2D EdgeTex        : register(t1);
SamplerState samplerState : register(s0);

struct PS_INPUT
{
    float2 uv0 : TEXCOORD0;
};

float3 adjustSaturation( float3 color, float sat )
{
    float grey = dot( color, float3( 0.299, 0.587, 0.114 ) );
    return lerp( float3( grey, grey, grey ), color, sat );
}

float4 main(
    PS_INPUT inPs,
    uniform float numBands,
    uniform float saturation,
    uniform float edgeDarkness
) : SV_Target
{
    float3 color = RT.Sample( samplerState, inPs.uv0 ).rgb;
    float  edge  = EdgeTex.Sample( samplerState, inPs.uv0 ).r;

    // Quantise into discrete cel bands
    color = floor( color * numBands + 0.5 ) / numBands;

    // Saturation boost
    color = adjustSaturation( color, saturation );

    // Edge darkening
    color *= lerp( 1.0, edgeDarkness, edge );

    return float4( color, 1.0 );
}
