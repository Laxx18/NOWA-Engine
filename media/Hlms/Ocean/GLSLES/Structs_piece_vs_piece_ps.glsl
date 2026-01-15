@piece( OceanMaterialDecl )
layout_constbuffer(binding = 1) uniform MaterialBuf
{
	// Ocean datablock colours
	vec4 deepColour;
	vec4 shallowColour;

	/* kD is already divided by PI to make it energy conserving.
	  (formula is finalDiffuse = NdotL * surfaceDiffuse / PI)
	*/
	vec4 kD; // kD.w is shadowConstantBias
	vec4 roughness;
	vec4 metalness;
	vec4 detailOffsetScale[4];

	@insertpiece( custom_materialBuffer )
} material;
@end

@piece( OceanInstanceDecl )
struct CellData
{
	//.x = numVertsPerLine
	//.y = lodLevel
	//.z = vao->getPrimitiveCount() / m_verticesPerLine - 2u
	//.w = skirtY (float)
	uvec4 numVertsPerLine;
	ivec4 xzTexPosBounds;		// XZXZ
	vec4 pos;					// .w contains 1.0 / xzTexPosBounds.z
	vec4 scale;					// .w contains 1.0 / xzTexPosBounds.w
	
	// Ocean-specific: time controls for wave movement
	vec4 oceanTime;
};

layout_constbuffer(binding = 2) uniform InstanceBuffer
{
	CellData cellData[256];
} instance;
@end

@piece( Ocean_VStoPS_block )
	// Ocean core data
	vec3 pos;
	vec3 normal;
	vec3 wpos;

	float  waveHeight;
	float  wavesIntensity;

	vec2 uv0;
	vec3 uv1;
	vec3 uv2;
	vec3 uv3;
	vec3 uv4;

	vec3 blendWeight;

	@foreach( hlms_num_shadow_map_lights, n )
		@property( !hlms_shadowmap@n_is_point_light )
			vec4 posL@n;@end @end
	@property( hlms_pssm_splits )float depth;@end
	@insertpiece( custom_VStoPS )
@end