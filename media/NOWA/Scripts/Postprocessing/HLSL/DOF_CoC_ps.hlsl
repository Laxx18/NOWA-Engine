//-------------------------------
// Depth of Field — CoC Pass
//-------------------------------

Texture2D<float4> RT       : register(t0);
Texture2D<float>  DepthTex : register(t1);

SamplerState samplerBilinear : register(s0);
SamplerState samplerPoint    : register(s1);

struct PS_INPUT { float2 uv0 : TEXCOORD0; };

float lineariseDepth( float raw, float2 pp, float farClip )
{
    float d = raw - pp.x;
    if ( abs(d) < 0.0001f ) d = 0.0001f;
    return (pp.y / d) * farClip;
}

float4 main(
    PS_INPUT inPs,
    uniform float2 projectionParams,
    uniform float  farClipDistance,
    uniform float  focusDistance,
    uniform float  nearBlurRange,
    uniform float  farBlurRange
) : SV_Target
{
    float3 color    = RT.Sample( samplerBilinear, inPs.uv0 ).rgb;
    float  rawDepth = DepthTex.Sample( samplerPoint, inPs.uv0 ).r;

    float isSky    = step( 0.9999f, rawDepth );
    float linDepth = lineariseDepth( rawDepth, projectionParams, farClipDistance );

    float cocFar  = saturate( (linDepth        - focusDistance) / max(farBlurRange,  0.001f) );
    float cocNear = saturate( (focusDistance   - linDepth)      / max(nearBlurRange, 0.001f) );

    float coc = max( cocFar, cocNear ) * (1.0f - isSky);

    return float4( color, coc );
}
