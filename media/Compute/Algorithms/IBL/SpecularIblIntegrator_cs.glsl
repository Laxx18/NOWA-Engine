
@insertpiece( SetCrossPlatformSettings )
@insertpiece( DeclUavCrossPlatform )

layout( local_size_x = @value( threads_per_group_x ),
		local_size_y = @value( threads_per_group_y ),
		local_size_z = @value( threads_per_group_z ) ) in;

vulkan_layout( ogre_t0 ) uniform textureCube convolutionSrc;
vulkan( layout( ogre_s0 ) uniform sampler EnvMapSampler );

@property( uav0_texture_type == TextureTypes_TypeCube )
	layout( vulkan( ogre_u0 ) vk_comma @insertpiece( uav0_pf_type ) )
	uniform restrict image2DArray lastResult;
@else
	layout( vulkan( ogre_u0 ) vk_comma @insertpiece( uav0_pf_type ) )
	uniform restrict image2D lastResult;
@end


vulkan( layout( ogre_P0 ) uniform Params { )
	uniform float4 params0;
	uniform float4 params1;
	uniform float4 params2;

	uniform float4 iblCorrection;
vulkan( }; )

#define p_convolutionSamplesOffset params0.x
#define p_convolutionSampleCount params0.y
#define p_convolutionMaxSamples params0.z
#define p_convolutionRoughness params0.w

#define p_convolutionMip params1.x
#define p_environmentScale params1.y

#define p_inputResolution params2.xy
#define p_outputResolution params2.zw

#define p_iblCorrection iblCorrection

@insertpiece( HeaderCS )

void main()
{
	@insertpiece( BodyCS )
}
