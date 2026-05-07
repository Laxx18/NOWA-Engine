#version ogre_glsl_ver_330

//-------------------------------
// Volumetric Light — Pass 3: Blend
//
// Additively blends the blurred god-ray contribution onto the original
// scene colour, with a colour tint and strength multiplier.
//
// Parameters:
//   godRayStrength - blend multiplier, 1.0 = full strength
//   tint           - RGB colour of the light shafts (e.g. warm sunlight = 1, 0.9, 0.7)
//
// RT      - original scene colour (texture unit 0)
// GodRays - radial-blur output   (texture unit 1)
//-------------------------------

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan_layout( ogre_t1 ) uniform texture2D GodRays;

vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform float godRayStrength;
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
    vec3 scene  = texture( vkSampler2D( RT,      samplerState ), inPs.uv0 ).rgb;
    vec3 rays   = texture( vkSampler2D( GodRays, samplerState ), inPs.uv0 ).rgb;

    // Additive blend — god rays add light on top of the scene
    fragColour = vec4( scene + rays * tint * godRayStrength, 1.0 );
}
