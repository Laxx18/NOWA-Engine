Texture2D<float4> rt0 : register(t0);
SamplerState samp0    : register(s0);

Texture2D<float> depthTex : register(t1);
SamplerState samp1        : register(s1);

struct PS_INPUT
{
    float2 uv0 : TEXCOORD0;
};

static float3 applySaturation( float3 c, float s )
{
    float l = dot( c, float3( 0.2126f, 0.7152f, 0.0722f ) );
    return lerp( l.xxx, c, s );
}

// Reverse-Z depth (Ogre-Next default on D3D11)
float depthToViewZ( float d, float nearC, float farC )
{
    d = saturate( d );
    float denom = nearC + d * (farC - nearC);
    return (nearC * farC) / max( denom, 1e-6 );
}

float4 main(
    PS_INPUT inPs,

    // --- old parameters ---
    uniform float underwaterFactor,
    uniform float timeSeconds,
    uniform float3 waterTint,
    uniform float distortion,
    uniform float contrast,
    uniform float saturation,
    uniform float vignette,

    // --- new depth fog parameters ---
    uniform float3 fogColour,
    uniform float  fogDensity,
    uniform float3 absorptionCoeff,
    uniform float  nearClip,
    uniform float  farClip
) : SV_Target0
{
    float2 uv = inPs.uv0;

    // ---------------------------------------------------
    // ORIGINAL squiggle (unchanged)
    // ---------------------------------------------------
    float n1 = sin( (uv.x + timeSeconds * 0.25f) * 40.0f );
    float n2 = sin( (uv.y + timeSeconds * 0.17f) * 35.0f );
    float n  = n1 * n2;
    float2 duv = float2( n, -n ) * (distortion * underwaterFactor);

    float3 col = rt0.Sample( samp0, uv + duv ).rgb;

    // ---------------------------------------------------
    // Depth fog
    // ---------------------------------------------------
    float d = depthTex.Sample( samp1, uv ).r;
    float viewZ = depthToViewZ( d, nearClip, farClip );

    float3 absorb    = exp( -absorptionCoeff * viewZ );
    float3 scattered = fogColour * (1.0f - absorb);
    col = col * absorb + scattered;

    float fog = 1.0f - exp( -fogDensity * viewZ );
    col = lerp( col, fogColour, saturate( fog ) );

    // ---------------------------------------------------
    // ORIGINAL grading (unchanged)
    // ---------------------------------------------------
    col = lerp( col, waterTint, 0.35f * underwaterFactor );

    col = (col - 0.5f) * lerp( 1.0f, contrast, underwaterFactor ) + 0.5f;

    col = applySaturation( col, lerp( 1.0f, saturation, underwaterFactor ) );

    float2 d2 = abs( uv - 0.5f ) * 2.0f;
    float v = saturate( 1.0f - dot( d2, d2 ) * vignette );
    col *= lerp( 1.0f, v, underwaterFactor );

    return float4( col, 1.0f );
}
