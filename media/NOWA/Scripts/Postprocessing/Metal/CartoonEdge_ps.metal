//-------------------------------
// Cartoon: Sobel edge detection pass
// Outputs a grayscale edge mask (bright = edge, dark = flat).
//-------------------------------

#include <metal_stdlib>
using namespace metal;

struct PS_INPUT
{
    float2 uv0;
};

struct Params
{
    float4 texelSize;     // (1/w, 1/h, w, h) from inverse_texture_size auto-param
    float  edgeThreshold;
    float  edgeStrength;
};

float luminance( float3 c )
{
    return dot( c, float3( 0.299, 0.587, 0.114 ) );
}

fragment float4 main_metal(
    PS_INPUT             inPs        [[stage_in]],
    texture2d<float>     RT          [[texture(0)]],
    sampler              samplerState [[sampler(0)]],
    constant Params     &params      [[buffer(PARAMETER_SLOT)]]
)
{
    float2 uv = inPs.uv0;
    float2 ts = params.texelSize.xy;

    float tl = luminance( RT.sample( samplerState, uv + float2( -ts.x,  ts.y ) ).rgb );
    float tm = luminance( RT.sample( samplerState, uv + float2(  0.0,   ts.y ) ).rgb );
    float tr = luminance( RT.sample( samplerState, uv + float2(  ts.x,  ts.y ) ).rgb );
    float ml = luminance( RT.sample( samplerState, uv + float2( -ts.x,  0.0  ) ).rgb );
    float mr = luminance( RT.sample( samplerState, uv + float2(  ts.x,  0.0  ) ).rgb );
    float bl = luminance( RT.sample( samplerState, uv + float2( -ts.x, -ts.y ) ).rgb );
    float bm = luminance( RT.sample( samplerState, uv + float2(  0.0,  -ts.y ) ).rgb );
    float br = luminance( RT.sample( samplerState, uv + float2(  ts.x, -ts.y ) ).rgb );

    float gx = -tl - 2.0 * ml - bl + tr + 2.0 * mr + br;
    float gy = -tl - 2.0 * tm - tr + bl + 2.0 * bm + br;

    float edge = sqrt( gx * gx + gy * gy );
    edge = step( params.edgeThreshold, edge ) * params.edgeStrength;

    return float4( edge, edge, edge, 1.0 );
}
