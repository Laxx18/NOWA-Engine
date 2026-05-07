#version ogre_glsl_ver_330

//-------------------------------
// Volumetric Light — Pass 2: Radial Blur (Crytek / Kawase God Rays)
//
// For each pixel, steps from the current UV position toward the sun's
// screen-space position, accumulating the sun-masked colour with an
// exponential per-step decay.  The accumulated result is the raw
// god-ray contribution before final blending with the scene.
//
// Parameters:
//   sunScreenPos  - sun position in UV space [0..1], updated per-frame from C++
//   decay         - per-step decay factor, e.g. 0.97 (tighter) … 0.99 (softer)
//   density       - fraction of the pixel→sun path sampled; 0.5–1.0
//   weight        - per-sample colour weight, e.g. 0.04
//   exposure      - final scale on the accumulated colour, e.g. 0.25
//-------------------------------

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform vec2  sunScreenPos;
    uniform float decay;
    uniform float density;
    uniform float weight;
    uniform float exposure;
vulkan( }; )

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
    vec2 uv0;
} inPs;

const int NUM_SAMPLES = 32;

void main()
{
    vec2  uv      = inPs.uv0;
    // Step vector from current pixel toward the sun, scaled by density
    vec2  delta   = (sunScreenPos - uv) * (density / float( NUM_SAMPLES ));
    vec2  sampleUV = uv;

    vec3  accumulated       = vec3( 0.0 );
    float illuminationDecay = 1.0;

    for ( int i = 0; i < NUM_SAMPLES; i++ )
    {
        sampleUV += delta;
        vec3 s    = texture( vkSampler2D( RT, samplerState ), sampleUV ).rgb;
        accumulated       += s * illuminationDecay * weight;
        illuminationDecay *= decay;
    }

    fragColour = vec4( accumulated * exposure, 1.0 );
}
