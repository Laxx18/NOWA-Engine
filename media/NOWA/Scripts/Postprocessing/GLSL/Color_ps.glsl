#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform sampler2D RT;
vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan_layout( location = 0 )
uniform vec3	color;

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

void main()
{
	float3 newColor = texture( vkSampler2D( RT, samplerState ), inPs.uv0 ) * color;
	return float4(newColor.xyz, 1.0);
}
