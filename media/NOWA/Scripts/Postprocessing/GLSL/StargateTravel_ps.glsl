#version ogre_glsl_ver_330

//-------------------------------
// Stargate Travel — Wormhole Tunnel Effect
//
// elapsedTime  — param_named_auto, updated by Ogre every frame
// startTimeSeconds — pushed ONCE by startTravel(). Sentinel 1e10 = not started.
// durationSeconds  — pushed once at postInit / setDuration
//
// When rawT < 0 the effect has not been started yet → passthrough.
//-------------------------------

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform float elapsedTime;       // param_named_auto time 1.0
    uniform float startTimeSeconds;  // sentinel 1e10 = not started
    uniform float durationSeconds;
    uniform float aspectRatio;
    uniform vec3  tunnelColor;
    uniform float ringFrequency;
    uniform float ringSpeed;
    uniform float distortionStrength;
    uniform float brightness;
vulkan( }; )

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block { vec2 uv0; } inPs;

const float TWO_PI = 6.28318530;

float ringLayer( float r, float travelProgress, float freqScale )
{
    float phase = r * ringFrequency * freqScale * TWO_PI
                - travelProgress * ringSpeed * TWO_PI;
    return pow( sin( phase ) * 0.5 + 0.5, 6.0 );
}

void main()
{
    vec3 sceneColor = texture( vkSampler2D( RT, samplerState ), inPs.uv0 ).rgb;

    // -----------------------------------------------------------------------
    // Compute raw (unclamped) progress.
    // Negative → effect not started yet → passthrough scene unchanged.
    // -----------------------------------------------------------------------
    float rawT = (elapsedTime - startTimeSeconds) / max( durationSeconds, 0.001 );

    if ( rawT < 0.0 )
    {
        fragColour = vec4( sceneColor, 1.0 );
        return;
    }

    float t = clamp( rawT, 0.0, 1.0 );

    // Centred, aspect-corrected UV
    vec2  uv    = inPs.uv0 * 2.0 - 1.0;
    uv.x       *= aspectRatio;
    float r     = length( uv );
    float angle = atan( uv.y, uv.x );

    // Phase ramps
    float entryFlash     = 1.0 - smoothstep( 0.0,  0.12, t );
    float exitFlash      = smoothstep( 0.88, 1.0,  t );
    float flash          = max( entryFlash, exitFlash );
    float tunnelFade     = smoothstep( 0.08, 0.20, t )
                         * (1.0 - smoothstep( 0.82, 0.95, t ));
    float travelProgress = clamp( (t - 0.12) / 0.76, 0.0, 1.0 );

    // Rings with chromatic aberration
    float abr   = distortionStrength * 0.04;
    float ringR = ringLayer( r * (1.0 + abr), travelProgress, 1.0 );
    float ringG = ringLayer( r,                travelProgress, 1.0 );
    float ringB = ringLayer( r * (1.0 - abr), travelProgress, 1.0 );

    float edgeFade = 1.0 - smoothstep( 0.65, 1.05, r );
    vec3  rings    = vec3( ringR, ringG, ringB )
                   * (tunnelColor + vec3( 0.1, 0.1, 0.3 ))
                   * edgeFade * 1.8;

    // Speed lines
    float linePhase = fract( (angle / TWO_PI + 0.5) * 28.0 + travelProgress * 0.15 );
    float speedLine = smoothstep( 0.42, 0.50, linePhase )
                    * smoothstep( 0.58, 0.50, linePhase );
    speedLine      *= (1.0 - r) * edgeFade;
    vec3 lines      = vec3( 0.7, 0.85, 1.0 ) * speedLine * 0.6;

    // Central glow
    float glow   = pow( max( 0.0, 1.0 - r * 1.1 ), 3.5 ) * 2.5;
    vec3  centre = vec3( 0.75, 0.90, 1.0 ) * glow;

    // Outer rim
    float rim  = smoothstep( 0.85, 0.95, r ) * smoothstep( 1.05, 0.95, r );
    vec3  edge = tunnelColor * rim * 1.5;

    // Assemble + flash composite
    vec3 tunnel = (rings + lines + centre + edge) * brightness;
    vec3 result = mix( sceneColor, tunnel, tunnelFade ) + vec3( flash );

    fragColour = vec4( min( result, vec3( 2.5 ) ), 1.0 );
}
