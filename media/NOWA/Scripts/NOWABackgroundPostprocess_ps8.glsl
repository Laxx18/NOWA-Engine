#version ogre_glsl_ver_330

vulkan( layout( ogre_s0 ) uniform sampler2D backgroundMap8;
vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan_layout( location = 0 ) uniform float speedX;
vulkan_layout( location = 0 ) uniform float speedY;

vulkan_layout( location = 0 ) out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

void main()
{
	// Apply scroll
	inPs.uv0.x += speedX;
	inPs.uv0.y += speedY;
	fragColour = texture( vkSampler2D( backgroundMap8, samplerState ), inPs.uv0 )
}
