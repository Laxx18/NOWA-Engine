//#include "SyntaxHighlightingMisc.h"

@insertpiece( SetCrossPlatformSettings )
@insertpiece( SetCompatibilityLayer )

struct VS_INPUT
{
@property( !iOS )
	ushort drawId	[[attribute(15)]];
@end
	@insertpiece( custom_vs_attributes )
};

struct PS_INPUT
{
	@insertpiece( Ocean_VStoPS_block )
	float4 gl_Position [[position]];
	@foreach( hlms_pso_clip_distances, n )
		float gl_ClipDistance [[clip_distance]] [@value( hlms_pso_clip_distances )];
	@end
};

@insertpiece( DefaultOceanHeaderVS )

vertex PS_INPUT main_metal
(
	VS_INPUT input [[stage_in]]
	, uint inVs_vertexId	[[vertex_id]]
	@property( iOS )
		, ushort instanceId [[instance_id]]
		, constant ushort &baseInstance [[buffer(15)]]
	@end
	// START UNIFORM DECLARATION
	@insertpiece( PassDecl )
	@insertpiece( OceanInstanceDecl )
	@property( hlms_shadowcaster )
		@insertpiece( MaterialDecl )
	@end
	@insertpiece( AtmosphereNprSkyDecl )
	, texture3d<midf, access::sample> terrainData [[texture(@value(terrainData))]]
	, texture2d<midf, access::sample> blendMap [[texture(@value(blendMap))]]
	, sampler terrainDataSampler [[sampler(@value(terrainData))]]
	, sampler blendMapSampler [[sampler(@value(blendMap))]]
	@insertpiece( custom_vs_uniformDeclaration )
	// END UNIFORM DECLARATION
)
{
	PS_INPUT outVs;

	@insertpiece( custom_vs_preExecution )
	@insertpiece( DefaultOceanBodyVS )
	@insertpiece( custom_vs_posExecution )

	return outVs;
}