#version ogre_glsl_ver_330

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
	vec2 uv0;
} inPs;

vulkan_layout( ogre_t0 ) uniform texture2D Keyhole_Mask;

vulkan( layout( ogre_s0 ) uniform sampler samplerState0 );

vulkan( layout( ogre_P0 ) uniform Params { )
uniform vec2 screenSize;
uniform vec2 center;
uniform float radius;
vulkan( }; )


void main()
{
    vec2 position = (inPs.uv0 * screenSize) - center;
	
	float normalizedRadius = radius * length(screenSize);

    float dist = length(position);
    float4 maskValue = texture(Keyhole_Mask, inPs.uv0).rgba;

    if ((direction == 0 && dist > normalizedRadius) || (direction == 1 && dist < normalizedRadius))
    {
        fragColour = vec4(0.0, 0.0, 0.0, 1.0);  // Outside the keyhole (black)
    }
    else
    {
        fragColour = vec4(1.0, 1.0, 1.0, 1.0) * maskValue;  // Inside the keyhole (white) modulated by the mask
    }
}
