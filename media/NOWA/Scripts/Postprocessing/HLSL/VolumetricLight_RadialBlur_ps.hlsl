//-------------------------------
// Volumetric Light — Pass 2: Radial Blur (Crytek / Kawase God Rays)
//-------------------------------

Texture2D<float4> RT          : register(t0);
SamplerState      samplerState : register(s0);

struct PS_INPUT
{
    float2 uv0 : TEXCOORD0;
};

static const int NUM_SAMPLES = 32;

float4 main(
    PS_INPUT inPs,
    uniform float2 sunScreenPos,
    uniform float  decay,
    uniform float  density,
    uniform float  weight,
    uniform float  exposure
) : SV_Target
{
    float2 uv       = inPs.uv0;
    float2 delta    = (sunScreenPos - uv) * (density / (float)NUM_SAMPLES);
    float2 sampleUV = uv;

    float3 accumulated       = float3( 0, 0, 0 );
    float  illuminationDecay = 1.0;

    [unroll]
    for ( int i = 0; i < NUM_SAMPLES; i++ )
    {
        sampleUV  += delta;
        float3 s   = RT.Sample( samplerState, sampleUV ).rgb;
        accumulated       += s * illuminationDecay * weight;
        illuminationDecay *= decay;
    }

    return float4( accumulated * exposure, 1.0 );
}
