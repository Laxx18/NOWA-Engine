//-------------------------------
// Depth of Field — CoC Pass
//-------------------------------

#include <metal_stdlib>
using namespace metal;

struct PS_INPUT { float2 uv0; };

struct Params
{
    float2 projectionParams;
    float  farClipDistance;
    float  focusDistance;
    float  nearBlurRange;
    float  farBlurRange;
    float  _pad0;
    float  _pad1;
};

static float lineariseDepth( float raw, float2 pp, float farClip )
{
    float d = raw - pp.x;
    if ( fabs(d) < 0.0001f ) d = 0.0001f;
    return (pp.y / d) * farClip;
}

fragment float4 main_metal(
    PS_INPUT              inPs            [[stage_in]],
    texture2d<float>      RT              [[texture(0)]],
    texture2d<float>      DepthTex        [[texture(1)]],
    sampler               samplerBilinear [[sampler(0)]],
    sampler               samplerPoint    [[sampler(1)]],
    constant Params      &params          [[buffer(PARAMETER_SLOT)]]
)
{
    float3 color    = RT.sample( samplerBilinear, inPs.uv0 ).rgb;
    float  rawDepth = DepthTex.sample( samplerPoint, inPs.uv0 ).r;

    float isSky    = step( 0.9999f, rawDepth );
    float linDepth = lineariseDepth( rawDepth, params.projectionParams, params.farClipDistance );

    float cocFar  = saturate( (linDepth             - params.focusDistance) / max(params.farBlurRange,  0.001f) );
    float cocNear = saturate( (params.focusDistance - linDepth)             / max(params.nearBlurRange, 0.001f) );

    float coc = max( cocFar, cocNear ) * (1.0f - isSky);

    return float4( color.x, color.y, color.z, coc );
}
