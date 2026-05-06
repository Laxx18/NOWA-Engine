//-------------------------------
// Cartoon: Sobel edge detection pass
// Outputs a grayscale edge mask (bright = edge, dark = flat).
//-------------------------------

Texture2D RT             : register(t0);
SamplerState samplerState : register(s0);

struct PS_INPUT
{
    float2 uv0 : TEXCOORD0;
};

float luminance( float3 c )
{
    return dot( c, float3( 0.299, 0.587, 0.114 ) );
}

float4 main(
    PS_INPUT inPs,
    uniform float4 texelSize,     // (1/w, 1/h, w, h) from inverse_texture_size auto-param
    uniform float  edgeThreshold,
    uniform float  edgeStrength
) : SV_Target
{
    float2 uv = inPs.uv0;
    float2 ts = texelSize.xy;

    float tl = luminance( RT.Sample( samplerState, uv + float2( -ts.x,  ts.y ) ).rgb );
    float tm = luminance( RT.Sample( samplerState, uv + float2(  0.0,   ts.y ) ).rgb );
    float tr = luminance( RT.Sample( samplerState, uv + float2(  ts.x,  ts.y ) ).rgb );
    float ml = luminance( RT.Sample( samplerState, uv + float2( -ts.x,  0.0  ) ).rgb );
    float mr = luminance( RT.Sample( samplerState, uv + float2(  ts.x,  0.0  ) ).rgb );
    float bl = luminance( RT.Sample( samplerState, uv + float2( -ts.x, -ts.y ) ).rgb );
    float bm = luminance( RT.Sample( samplerState, uv + float2(  0.0,  -ts.y ) ).rgb );
    float br = luminance( RT.Sample( samplerState, uv + float2(  ts.x, -ts.y ) ).rgb );

    float gx = -tl - 2.0 * ml - bl + tr + 2.0 * mr + br;
    float gy = -tl - 2.0 * tm - tr + bl + 2.0 * bm + br;

    float edge = sqrt( gx * gx + gy * gy );
    edge = step( edgeThreshold, edge ) * edgeStrength;

    return float4( edge, edge, edge, 1.0 );
}
