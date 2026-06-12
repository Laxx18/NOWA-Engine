#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform textureCube skyCubemap;
vulkan( layout( ogre_s0 ) uniform sampler cubeSampler );

vulkan( layout( ogre_P0 ) uniform Params { )
	// Scales the LDR cubemap into the HDR luminance range of the scene.
	uniform float hdrSkyPower;
vulkan( }; )

vulkan_layout( location = 0 )
in block
{
    vec3 cameraDir;
} inPs;

vulkan_layout( location = 0 ) out vec4 fragColour;

void main()
{
	//Cubemaps are left-handed
	fragColour = texture( vkSamplerCube( skyCubemap, cubeSampler ),
						  vec3( inPs.cameraDir.xy, -inPs.cameraDir.z ) );
	fragColour.xyz *= hdrSkyPower;
}