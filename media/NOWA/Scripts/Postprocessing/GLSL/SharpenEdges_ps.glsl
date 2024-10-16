#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan( layout( ogre_P0 ) uniform Params { )
	uniform vec2 vTexelSize;
	uniform float weight;
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
	const vec2 usedTexelED[8] = vec2[8]
	(
		vec2(-1, -1),
		vec2( 0, -1),
		vec2( 1, -1),
		vec2(-1,  0),
		vec2( 1,  0),
		vec2(-1,  1),
		vec2( 0,  1),
		vec2( 1,  1)
	);

	vec4 cAvgColor = weight * texture( vkSampler2D( RT, samplerState ), inPs.uv0 );

	for(int t=0; t<8; t++)
	{
		cAvgColor -= texture( vkSampler2D( RT, samplerState ),
							  inPs.uv0 + vTexelSize * usedTexelED[t] );
	}

	fragColour = cAvgColor;
}
