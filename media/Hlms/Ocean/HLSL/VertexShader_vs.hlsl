@insertpiece( SetCrossPlatformSettings )
@insertpiece( SetCompatibilityLayer )

struct VS_INPUT
{
	uint vertexId	: SV_VertexID;
	uint drawId		: DRAWID;
	@insertpiece( custom_vs_attributes )
};

struct PS_INPUT
{
	@insertpiece( Ocean_VStoPS_block )
	float4 gl_Position: SV_Position;

	@pdiv( full_pso_clip_distances, hlms_pso_clip_distances, 4 )
	@pmod( partial_pso_clip_distances, hlms_pso_clip_distances, 4 )
	@foreach( full_pso_clip_distances, n )
		float4 gl_ClipDistance@n : SV_ClipDistance@n;
	@end
	@property( partial_pso_clip_distances )
		float@value( partial_pso_clip_distances ) gl_ClipDistance@value( full_pso_clip_distances ) : SV_ClipDistance@value( full_pso_clip_distances );
	@end
};

@insertpiece( DefaultOceanHeaderVS )
@insertpiece( custom_vs_uniformDeclaration )

// START UNIFORM DECLARATION
Texture3D terrainData : register(t@value(terrainData));
Texture2D blendMap    : register(t@value(blendMap));
// END UNIFORM DECLARATION

// Samplers
SamplerState samplerState@value(terrainData) : register(s@value(terrainData));
SamplerState samplerState@value(blendMap)    : register(s@value(blendMap));

PS_INPUT main( VS_INPUT input )
{
	PS_INPUT outVs;

	@insertpiece( custom_vs_preExecution )
	@insertpiece( DefaultOceanBodyVS )
	@insertpiece( custom_vs_posExecution )

	return outVs;
}