#version ogre_glsl_ver_330

//-------------------------------
// Light Shafts — Pass 3: Blend
//
// Additively composites the shaft contribution onto the scene.
// Identical to VolumetricLightBlend — kept separate so both effects
// can coexist with independent settings.
//-------------------------------

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan_layout( ogre_t1 ) uniform texture2D ShaftTex;

vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform float shaftStrength;
    uniform vec3  tint;
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
    vec3 scene  = texture( vkSampler2D( RT,       samplerState ), inPs.uv0 ).rgb;
    vec3 shafts = texture( vkSampler2D( ShaftTex, samplerState ), inPs.uv0 ).rgb;

    fragColour = vec4( scene + shafts * tint * shaftStrength, 1.0 );
}
