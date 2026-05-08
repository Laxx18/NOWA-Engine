//-------------------------------
// Outline / Silhouette Effect — depth-based Sobel (Metal)
//-------------------------------

#include <metal_stdlib>
using namespace metal;

struct PS_INPUT { float2 uv0; };

struct Params
{
    float2 projectionParams;
    float  farClipDistance;
    float  _pad0;
    float4 texelSize;
    float4 outlineColor;    // Metal pads float3 → float4, ignore .w
    float  outlineThickness;
    float  depthThreshold;
    float  outlineStrength;
    float  _pad1;
};

static float lineariseDepth( float raw, float2 pp )
{
    float d = raw - pp.x;
    if ( fabs(d) < 0.0001f ) d = 0.0001f;
    return saturate( pp.y / d );
}

fragment float4 main_metal(
    PS_INPUT             inPs            [[stage_in]],
    texture2d<float>     RT              [[texture(0)]],
    texture2d<float>     DepthTex        [[texture(1)]],
    sampler              samplerBilinear [[sampler(0)]],
    sampler              samplerPoint    [[sampler(1)]],
    constant Params     &params          [[buffer(PARAMETER_SLOT)]]
)
{
    float3 sceneColor = RT.sample( samplerBilinear, inPs.uv0 ).rgb;

    float2 ts = params.texelSize.xy * params.outlineThickness;

    float tl = lineariseDepth( DepthTex.sample( samplerPoint, inPs.uv0 + float2(-ts.x,  ts.y) ).r, params.projectionParams );
    float tm = lineariseDepth( DepthTex.sample( samplerPoint, inPs.uv0 + float2( 0.0f,  ts.y) ).r, params.projectionParams );
    float tr = lineariseDepth( DepthTex.sample( samplerPoint, inPs.uv0 + float2( ts.x,  ts.y) ).r, params.projectionParams );
    float ml = lineariseDepth( DepthTex.sample( samplerPoint, inPs.uv0 + float2(-ts.x,  0.0f) ).r, params.projectionParams );
    float mr = lineariseDepth( DepthTex.sample( samplerPoint, inPs.uv0 + float2( ts.x,  0.0f) ).r, params.projectionParams );
    float bl = lineariseDepth( DepthTex.sample( samplerPoint, inPs.uv0 + float2(-ts.x, -ts.y) ).r, params.projectionParams );
    float bm = lineariseDepth( DepthTex.sample( samplerPoint, inPs.uv0 + float2( 0.0f, -ts.y) ).r, params.projectionParams );
    float br = lineariseDepth( DepthTex.sample( samplerPoint, inPs.uv0 + float2( ts.x, -ts.y) ).r, params.projectionParams );

    float gx = -tl - 2.0f * ml - bl + tr + 2.0f * mr + br;
    float gy = -tl - 2.0f * tm - tr + bl + 2.0f * bm + br;

    float gradient = sqrt( gx * gx + gy * gy );
    float edge     = step( params.depthThreshold, gradient ) * params.outlineStrength;

    float3 result = mix( sceneColor, params.outlineColor.xyz, edge );
    return float4( result.x, result.y, result.z, 1.0f );
}
