#version ogre_glsl_ver_330

//-------------------------------
// Volumetric Light — Pass 1: Sun Mask
//
// Isolates bright pixels in the vicinity of the sun's screen-space position.
// The radial blur pass will smear these bright areas back toward the
// surrounding scene pixels, creating the illusion of light shafts.
//
// Parameters:
//   sunScreenPos  - sun position in UV space [0..1]
//   sunThreshold  - luminance threshold; only pixels above this cast rays
//   sunRadius     - screen-space radius beyond which the mask fades to zero
//-------------------------------

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform vec2  sunScreenPos;
    uniform float sunThreshold;
    uniform float sunRadius;
vulkan( }; )

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
    vec2 uv0;
} inPs;

void main()
{
    vec3  color = texture( vkSampler2D( RT, samplerState ), inPs.uv0 ).rgb;
    float lum   = dot( color, vec3( 0.2126, 0.7152, 0.0722 ) );

    // Hard brightness gate — only pixels above threshold contribute
    float brightMask = step( sunThreshold, lum );

    // Soft radial falloff from sun centre
    float sunDist = length( inPs.uv0 - sunScreenPos );
    float radialMask = 1.0 - smoothstep( 0.0, sunRadius, sunDist );

    float mask = brightMask * (0.4 + radialMask * 0.6);

    fragColour = vec4( color * mask, 1.0 );
}
