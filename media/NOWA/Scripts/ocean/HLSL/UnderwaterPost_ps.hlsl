// Minimal underwater post effect for Ogre-Next compositor quad
// Input: rt0 (scene colour)
// Params are provided via material script (default_params / param_named_auto)

Texture2D<float4> rt0 : register(t0);
SamplerState samp0    : register(s0);

struct PS_INPUT
{
    float2 uv0 : TEXCOORD0;
};

static float3 applySaturation( float3 c, float s )
{
    float l = dot( c, float3( 0.2126f, 0.7152f, 0.0722f ) );
    return lerp( l.xxx, c, s );
}

float4 main(
    PS_INPUT inPs,
    uniform float underwaterFactor,
    uniform float timeSeconds,
    uniform float3 waterTint,
    uniform float distortion,
    uniform float contrast,
    uniform float saturation,
    uniform float vignette
) : SV_Target0
{
    float2 uv = inPs.uv0;

    // Cheap moving distortion (no extra textures)
    float n1 = sin( (uv.x + timeSeconds * 0.25f) * 40.0f );
    float n2 = sin( (uv.y + timeSeconds * 0.17f) * 35.0f );
    float n  = n1 * n2;
    float2 duv = float2( n, -n ) * (distortion * underwaterFactor);

    float3 col = rt0.Sample( samp0, uv + duv ).rgb;

    // Absorption tint
    col = lerp( col, waterTint, 0.35f * underwaterFactor );

    // Contrast around mid grey
    col = (col - 0.5f) * lerp( 1.0f, contrast, underwaterFactor ) + 0.5f;

    // Saturation
    col = applySaturation( col, lerp( 1.0f, saturation, underwaterFactor ) );

    // Vignette
    float2 d = abs( uv - 0.5f ) * 2.0f;
    float v = saturate( 1.0f - dot( d, d ) * vignette );
    col *= lerp( 1.0f, v, underwaterFactor );

    return float4( col, 1.0f );
}
