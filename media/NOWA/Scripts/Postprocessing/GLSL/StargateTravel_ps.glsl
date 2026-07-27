#version ogre_glsl_ver_330

//-------------------------------
// Stargate Travel — Wormhole Tunnel Effect
//
// Time phases (t in [0..1]):
//   [0.00 .. 0.12]  Entry flash:  white burst, tunnel snaps open
//   [0.08 .. 0.85]  Travel:       blue rings converging to bright centre,
//                                 speed lines, chromatic aberration
//   [0.85 .. 1.00]  Exit flash:   tunnel collapses, white burst out
//
// Time is computed ENTIRELY in the shader:
//
//   elapsedTime      auto-param (param_named_auto elapsedTime time 1.0)
//                    Ogre updates this every frame — zero C++ push per frame.
//
//   startTimeSeconds pushed ONCE by C++ when startTravel() is called.
//                    A large sentinel value (1e10) means "not started yet".
//
//   durationSeconds  pushed ONCE at postInit() and when the variant changes.
//
// The shader computes:  t = clamp((elapsedTime - startTimeSeconds) / duration)
// The C++ update() loop handles ONLY game-thread bookkeeping for the callback.
//-------------------------------

vulkan_layout( ogre_t0 ) uniform texture2D RT;   // scene colour (flash composite)
vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform float elapsedTime;       // param_named_auto  — Ogre sets this every frame
    uniform float startTimeSeconds;  // pushed once on startTravel()
    uniform float durationSeconds;   // pushed once at postInit / setDuration()
    uniform float aspectRatio;       // pushed once at postInit
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
    float wave  = sin( phase ) * 0.5 + 0.5;
    return pow( wave, 6.0 );
}

void main()
{
    // -----------------------------------------------------------------------
    // Compute normalised time [0..1] entirely on GPU
    // -----------------------------------------------------------------------
    float t = clamp( (elapsedTime - startTimeSeconds) / max(durationSeconds, 0.001),
                     0.0, 1.0 );

    // -----------------------------------------------------------------------
    // Centred, aspect-corrected UV
    // -----------------------------------------------------------------------
    vec2  uv    = inPs.uv0 * 2.0 - 1.0;
    uv.x       *= aspectRatio;
    float r     = length( uv );
    float angle = atan( uv.y, uv.x );

    // -----------------------------------------------------------------------
    // Phase ramps
    // -----------------------------------------------------------------------
    float entryFlash     = 1.0 - smoothstep( 0.0,  0.12, t );
    float exitFlash      = smoothstep( 0.88, 1.0,  t );
    float flash          = max( entryFlash, exitFlash );
    float tunnelFade     = smoothstep( 0.08, 0.20, t )
                         * (1.0 - smoothstep( 0.82, 0.95, t ));
    float travelProgress = clamp( (t - 0.12) / 0.76, 0.0, 1.0 );

    // -----------------------------------------------------------------------
    // Rings — three layers with chromatic aberration offsets
    // -----------------------------------------------------------------------
    float abr   = distortionStrength * 0.04;
    float ringR = ringLayer( r * (1.0 + abr), travelProgress, 1.0 );
    float ringG = ringLayer( r,                travelProgress, 1.0 );
    float ringB = ringLayer( r * (1.0 - abr), travelProgress, 1.0 );

    float edgeFade = 1.0 - smoothstep( 0.65, 1.05, r );
    vec3  rings    = vec3( ringR, ringG, ringB )
                   * (tunnelColor + vec3( 0.1, 0.1, 0.3 ))
                   * edgeFade * 1.8;

    // -----------------------------------------------------------------------
    // Speed lines
    // -----------------------------------------------------------------------
    float linePhase = fract( (angle / TWO_PI + 0.5) * 28.0
                           + travelProgress * 0.15 );
    float speedLine = smoothstep( 0.42, 0.50, linePhase )
                    * smoothstep( 0.58, 0.50, linePhase );
    speedLine      *= (1.0 - r) * edgeFade;
    vec3 lines      = vec3( 0.7, 0.85, 1.0 ) * speedLine * 0.6;

    // -----------------------------------------------------------------------
    // Central convergence glow
    // -----------------------------------------------------------------------
    float glow   = pow( max( 0.0, 1.0 - r * 1.1 ), 3.5 ) * 2.5;
    vec3  centre = vec3( 0.75, 0.90, 1.0 ) * glow;

    // -----------------------------------------------------------------------
    // Outer rim
    // -----------------------------------------------------------------------
    float rim  = smoothstep( 0.85, 0.95, r ) * smoothstep( 1.05, 0.95, r );
    vec3  edge = tunnelColor * rim * 1.5;

    // -----------------------------------------------------------------------
    // Assemble + flash composite
    // -----------------------------------------------------------------------
    vec3 tunnel     = (rings + lines + centre + edge) * brightness;
    vec3 sceneColor = texture( vkSampler2D( RT, samplerState ), inPs.uv0 ).rgb;
    vec3 result     = mix( sceneColor, tunnel, tunnelFade ) + vec3( flash );

    fragColour = vec4( min( result, vec3( 2.5 ) ), 1.0 );
}
