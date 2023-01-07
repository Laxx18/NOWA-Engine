#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan_layout( location = 0 )
uniform vec3	color;

vulkan_layout( location = 0 )
out vec4 fragColour;

in block
{
	vec2 uv0;
} inPs;

void main()
{
	float greyscale = dot( texture( vkSampler2D( RT, samplerState ), inPs.uv0 ).xyz, color );
	fragColour = vec4(greyscale, greyscale, greyscale, 1.0);
}
