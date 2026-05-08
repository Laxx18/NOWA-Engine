//-------------------------------
// Depth of Field — Directional Blur
// Shared by Gaussian (H+V) and Hexagonal Bokeh (3 directions).
//-------------------------------

Texture2D<float4> RT          : register(t0);
SamplerState      samplerState : register(s0);

struct PS_INPUT { float2 uv0 : TEXCOORD0; };

float4 main(
    PS_INPUT inPs,
    uniform float2 blurDir,
    uniform float  blurRadius,
    uniform float  numSamples
) : SV_Target
{
    float4 center = RT.Sample( samplerState, inPs.uv0 );
    float  coc    = center.a;

    if ( coc < 0.005f )
        return center;

    float stepSize = (blurRadius * coc) / numSamples;
    float2 stepVec = blurDir * stepSize;

    float4 accumulated = float4(0,0,0,0);
    float  totalWeight = 0.0f;

    int iSamples = (int)numSamples;

    [loop]
    for ( int i = -iSamples; i <= iSamples; i++ )
    {
        float2 sampleUV = inPs.uv0 + stepVec * (float)i;
        float4 s        = RT.Sample( samplerState, sampleUV );
        float  w        = max( s.a, coc * 0.5f );
        accumulated    += s * w;
        totalWeight    += w;
    }

    return accumulated / max( totalWeight, 0.0001f );
}
