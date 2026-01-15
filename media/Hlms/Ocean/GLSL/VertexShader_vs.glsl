@insertpiece( SetCrossPlatformSettings )
@insertpiece( SetCompatibilityLayer )

out gl_PerVertex
{
	vec4 gl_Position;
@property( hlms_pso_clip_distances && !hlms_emulate_clip_distances )
	float gl_ClipDistance[@value(hlms_pso_clip_distances)];
@end
};

layout(std140) uniform;

@insertpiece( DefaultOceanHeaderVS )
@insertpiece( custom_vs_uniformDeclaration )

@property( GL_ARB_base_instance )
	vulkan_layout( OGRE_DRAWID ) in uint drawId;
@end

@insertpiece( custom_vs_attributes )

vulkan_layout( location = 0 )
out block
{
	@insertpiece( Ocean_VStoPS_block )
} outVs;

// START UNIFORM GL DECLARATION
vulkan_layout( ogre_t@value(terrainData) ) midf_tex uniform texture3D terrainData;
vulkan_layout( ogre_t@value(blendMap) ) midf_tex uniform texture2D blendMap;

@property( !GL_ARB_base_instance )uniform uint baseInstance;@end
// END UNIFORM GL DECLARATION

vulkan( layout( ogre_s@value(terrainData) ) uniform sampler samplerState@value(terrainData) );
vulkan( layout( ogre_s@value(blendMap) ) uniform sampler samplerState@value(blendMap) );

void main()
{
@property( !GL_ARB_base_instance )
	uint drawId = baseInstance + uint( gl_InstanceID );
@end
	@insertpiece( custom_vs_preExecution )
	@insertpiece( DefaultOceanBodyVS )
	@insertpiece( custom_vs_posExecution )
}