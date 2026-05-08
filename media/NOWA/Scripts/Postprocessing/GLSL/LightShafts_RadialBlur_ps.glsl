#version ogre_glsl_ver_330

//-------------------------------
// Light Shafts — Pass 2: Radial Blur
//
// Identical algorithm to VolumetricLight but with an extra sharpness
// parameter: higher sharpness boosts the contrast of the accumulated
// sample, making shaft edges crisper vs the soft glow of VolumetricLight.
//-------------------------------

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform vec2  sunScreenPos;
    uniform float decay;
    uniform float density;
    uniform float weight;
    uniform float exposure;
    uniform float shaftSharpness;  // pow() contrast boost on accumulated result [1..4]
vulkan( }; )

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
    vec2 uv0;
} inPs;

const int NUM_SAMPLES = 48;  // more samples than VolumetricLight for cleaner shaft edges

void main()
{
    vec2  uv       = inPs.uv0;
    vec2  delta    = (sunScreenPos - uv) * (density / float( NUM_SAMPLES ));
    vec2  sampleUV = uv;

    vec3  accumulated       = vec3( 0.0 );
    float illuminationDecay = 1.0;

    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        sampleUV  += delta;
        vec3 s     = texture( vkSampler2D( RT, samplerState ), sampleUV ).rgb;
        accumulated       += s * illuminationDecay * weight;
        illuminationDecay *= decay;
    }

    accumulated *= exposure;

    // Sharpness: pow contrast boost makes soft glow → distinct shafts
    accumulated = pow( clamp( accumulated, 0.0, 1.0 ), vec3( 1.0 / max( shaftSharpness, 0.001 ) ) );

    fragColour = vec4( accumulated, 1.0 );
}
