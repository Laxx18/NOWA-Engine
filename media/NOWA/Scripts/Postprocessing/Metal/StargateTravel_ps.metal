//-------------------------------
// Stargate Travel — Wormhole Tunnel Effect (Metal)
// Time computed in shader from elapsedTime auto-param + startTimeSeconds.
//-------------------------------

#include <metal_stdlib>
using namespace metal;

struct PS_INPUT { float2 uv0; };

struct Params
{
    float  elapsedTime;
    float  startTimeSeconds;
    float  durationSeconds;
    float  aspectRatio;
    float4 tunnelColor;       // Metal pads float3 → float4, ignore .w
    float  ringFrequency;
    float  ringSpeed;
    float  distortionStrength;
    float  brightness;
};

static float ringLayer( float r, float travelProgress,
                        float ringFrequency, float ringSpeed, float freqScale )
{
    const float TWO_PI = 6.28318530f;
    float phase = r * ringFrequency * freqScale * TWO_PI
                - travelProgress * ringSpeed * TWO_PI;
    float wave  = sin( phase ) * 0.5f + 0.5f;
    return pow( wave, 6.0f );
}

fragment float4 main_metal(
    PS_INPUT             inPs        [[stage_in]],
    texture2d<float>     RT          [[texture(0)]],
    sampler              samplerState [[sampler(0)]],
    constant Params     &p           [[buffer(PARAMETER_SLOT)]]
)
{
    const float TWO_PI = 6.28318530f;

    float t = saturate( (p.elapsedTime - p.startTimeSeconds) / max(p.durationSeconds, 0.001f) );

    float2 uv   = inPs.uv0 * 2.0f - 1.0f;
    uv.x       *= p.aspectRatio;
    float r     = length( uv );
    float angle = atan2( uv.y, uv.x );

    float entryFlash     = 1.0f - smoothstep( 0.0f,  0.12f, t );
    float exitFlash      = smoothstep( 0.88f, 1.0f,  t );
    float flash          = max( entryFlash, exitFlash );
    float tunnelFade     = smoothstep( 0.08f, 0.20f, t )
                         * (1.0f - smoothstep( 0.82f, 0.95f, t ));
    float travelProgress = saturate( (t - 0.12f) / 0.76f );

    float abr   = p.distortionStrength * 0.04f;
    float ringR = ringLayer( r * (1.0f + abr), travelProgress, p.ringFrequency, p.ringSpeed, 1.0f );
    float ringG = ringLayer( r,                 travelProgress, p.ringFrequency, p.ringSpeed, 1.0f );
    float ringB = ringLayer( r * (1.0f - abr), travelProgress, p.ringFrequency, p.ringSpeed, 1.0f );

    float3 tc       = p.tunnelColor.xyz;
    float  edgeFade = 1.0f - smoothstep( 0.65f, 1.05f, r );
    float3 rings    = float3( ringR, ringG, ringB )
                    * (tc + float3( 0.1f, 0.1f, 0.3f ))
                    * edgeFade * 1.8f;

    float  linePhase = fract( (angle / TWO_PI + 0.5f) * 28.0f + travelProgress * 0.15f );
    float  speedLine = smoothstep( 0.42f, 0.50f, linePhase )
                     * smoothstep( 0.58f, 0.50f, linePhase );
    speedLine       *= (1.0f - r) * edgeFade;
    float3 lines     = float3( 0.7f, 0.85f, 1.0f ) * speedLine * 0.6f;

    float  glow   = pow( max( 0.0f, 1.0f - r * 1.1f ), 3.5f ) * 2.5f;
    float3 centre = float3( 0.75f, 0.90f, 1.0f ) * glow;

    float  rim  = smoothstep( 0.85f, 0.95f, r ) * smoothstep( 1.05f, 0.95f, r );
    float3 edge = tc * rim * 1.5f;

    float3 tunnel     = (rings + lines + centre + edge) * p.brightness;
    float3 sceneColor = RT.sample( samplerState, inPs.uv0 ).rgb;
    float3 result     = mix( sceneColor, tunnel, tunnelFade ) + float3( flash );

    result = min( result, float3( 2.5f ) );
    return float4( result.x, result.y, result.z, 1.0f );
}
