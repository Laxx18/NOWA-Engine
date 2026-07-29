//-------------------------------
// Stargate Travel — Wormhole Tunnel Effect (HLSL)
// rawT < 0 → effect not started → passthrough scene.
//-------------------------------

Texture2D<float4> RT          : register(t0);
SamplerState      samplerState : register(s0);

struct PS_INPUT { float2 uv0 : TEXCOORD0; };

static const float TWO_PI = 6.28318530f;

static float ringLayer( float r, float travelProgress,
                        float ringFrequency, float ringSpeed, float freqScale )
{
    float phase = r * ringFrequency * freqScale * TWO_PI
                - travelProgress * ringSpeed * TWO_PI;
    return pow( sin( phase ) * 0.5f + 0.5f, 6.0f );
}

float4 main(
    PS_INPUT inPs,
    uniform float  elapsedTime,
    uniform float  startTimeSeconds,
    uniform float  durationSeconds,
    uniform float  aspectRatio,
    uniform float3 tunnelColor,
    uniform float  ringFrequency,
    uniform float  ringSpeed,
    uniform float  distortionStrength,
    uniform float  brightness
) : SV_Target
{
    float3 sceneColor = RT.Sample( samplerState, inPs.uv0 ).rgb;

    // Negative rawT means effect not started → passthrough
    float rawT = (elapsedTime - startTimeSeconds) / max( durationSeconds, 0.001f );
    if ( rawT < 0.0f )
        return float4( sceneColor, 1.0f );

    float t = saturate( rawT );

    float2 uv   = inPs.uv0 * 2.0f - 1.0f;
    uv.x       *= aspectRatio;
    float r     = length( uv );
    float angle = atan2( uv.y, uv.x );

    float entryFlash     = 1.0f - smoothstep( 0.0f,  0.12f, t );
    float exitFlash      = smoothstep( 0.88f, 1.0f,  t );
    float flash          = max( entryFlash, exitFlash );
    float tunnelFade     = smoothstep( 0.08f, 0.20f, t )
                         * (1.0f - smoothstep( 0.82f, 0.95f, t ));
    float travelProgress = saturate( (t - 0.12f) / 0.76f );

    float abr   = distortionStrength * 0.04f;
    float ringR = ringLayer( r * (1.0f + abr), travelProgress, ringFrequency, ringSpeed, 1.0f );
    float ringG = ringLayer( r,                 travelProgress, ringFrequency, ringSpeed, 1.0f );
    float ringB = ringLayer( r * (1.0f - abr), travelProgress, ringFrequency, ringSpeed, 1.0f );

    float  edgeFade = 1.0f - smoothstep( 0.65f, 1.05f, r );
    float3 rings    = float3( ringR, ringG, ringB )
                    * (tunnelColor + float3( 0.1f, 0.1f, 0.3f ))
                    * edgeFade * 1.8f;

    float  linePhase = frac( (angle / TWO_PI + 0.5f) * 28.0f + travelProgress * 0.15f );
    float  speedLine = smoothstep( 0.42f, 0.50f, linePhase )
                     * smoothstep( 0.58f, 0.50f, linePhase );
    speedLine       *= (1.0f - r) * edgeFade;
    float3 lines     = float3( 0.7f, 0.85f, 1.0f ) * speedLine * 0.6f;

    float  glow   = pow( max( 0.0f, 1.0f - r * 1.1f ), 3.5f ) * 2.5f;
    float3 centre = float3( 0.75f, 0.90f, 1.0f ) * glow;

    float  rim  = smoothstep( 0.85f, 0.95f, r ) * smoothstep( 1.05f, 0.95f, r );
    float3 edge = tunnelColor * rim * 1.5f;

    float3 tunnel = (rings + lines + centre + edge) * brightness;
    float3 result = lerp( sceneColor, tunnel, tunnelFade ) + float3( flash, flash, flash );

    return float4( min( result, float3(2.5f, 2.5f, 2.5f) ), 1.0f );
}
