//-------------------------------
// Outline / Silhouette Effect — depth-based Sobel
//-------------------------------

Texture2D<float4> RT       : register(t0);
Texture2D<float>  DepthTex : register(t1);

SamplerState samplerBilinear : register(s0);
SamplerState samplerPoint    : register(s1);

struct PS_INPUT { float2 uv0 : TEXCOORD0; };

static float lineariseDepth( float raw, float2 pp )
{
    float d = raw - pp.x;
    if ( abs(d) < 0.0001f ) d = 0.0001f;
    return saturate( pp.y / d );
}

float4 main(
    PS_INPUT inPs,
    uniform float2 projectionParams,
    uniform float  farClipDistance,
    uniform float4 texelSize,
    uniform float3 outlineColor,
    uniform float  outlineThickness,
    uniform float  depthThreshold,
    uniform float  outlineStrength
) : SV_Target
{
    float3 sceneColor = RT.Sample( samplerBilinear, inPs.uv0 ).rgb;

    float2 ts = texelSize.xy * outlineThickness;

    float tl = lineariseDepth( DepthTex.Sample( samplerPoint, inPs.uv0 + float2(-ts.x,  ts.y) ).r, projectionParams );
    float tm = lineariseDepth( DepthTex.Sample( samplerPoint, inPs.uv0 + float2( 0.0f,  ts.y) ).r, projectionParams );
    float tr = lineariseDepth( DepthTex.Sample( samplerPoint, inPs.uv0 + float2( ts.x,  ts.y) ).r, projectionParams );
    float ml = lineariseDepth( DepthTex.Sample( samplerPoint, inPs.uv0 + float2(-ts.x,  0.0f) ).r, projectionParams );
    float mr = lineariseDepth( DepthTex.Sample( samplerPoint, inPs.uv0 + float2( ts.x,  0.0f) ).r, projectionParams );
    float bl = lineariseDepth( DepthTex.Sample( samplerPoint, inPs.uv0 + float2(-ts.x, -ts.y) ).r, projectionParams );
    float bm = lineariseDepth( DepthTex.Sample( samplerPoint, inPs.uv0 + float2( 0.0f, -ts.y) ).r, projectionParams );
    float br = lineariseDepth( DepthTex.Sample( samplerPoint, inPs.uv0 + float2( ts.x, -ts.y) ).r, projectionParams );

    float gx = -tl - 2.0f * ml - bl + tr + 2.0f * mr + br;
    float gy = -tl - 2.0f * tm - tr + bl + 2.0f * bm + br;

    float gradient = sqrt( gx * gx + gy * gy );
    float edge     = step( depthThreshold, gradient ) * outlineStrength;

    float3 result = lerp( sceneColor, outlineColor, edge );
    return float4( result, 1.0f );
}
