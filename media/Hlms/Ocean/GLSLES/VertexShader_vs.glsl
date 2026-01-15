@insertpiece( SetCrossPlatformSettings )
@insertpiece( SetCompatibilityLayer )

@property( GL3+ )
out gl_PerVertex
{
	vec4 gl_Position;
};
@end

layout(std140) uniform;

@property( GL_ARB_base_instance )
	in uint drawId;
@end

@insertpiece( custom_vs_attributes )

out block
{
@insertpiece( Ocean_VStoPS_block )
} outVs;

// START UNIFORM DECLARATION
@insertpiece( PassDecl )
@insertpiece( OceanInstanceDecl )
uniform sampler3D terrainData;
uniform sampler2D blendMap;
@insertpiece( custom_vs_uniformDeclaration )
@property( !GL_ARB_base_instance )uniform uint baseInstance;@end
// END UNIFORM DECLARATION

@piece( VertexOceanTransform )
	// Lighting is in view space
	outVs.pos = ( vec4(worldPos.xyz, 1.0f) * passBuf.view ).xyz;
@property( !hlms_dual_paraboloid_mapping )
	gl_Position = vec4(worldPos.xyz, 1.0f) * passBuf.viewProj;@end
@property( hlms_dual_paraboloid_mapping )
	// Dual Paraboloid Mapping
	gl_Position.w	= 1.0f;
	gl_Position.xyz	= outVs.pos;
	float L = length( gl_Position.xyz );
	gl_Position.z	+= 1.0f;
	gl_Position.xy	/= gl_Position.z;
	gl_Position.z	= (L - NearPlane) / (FarPlane - NearPlane);@end
@end

void main()
{
@property( !GL_ARB_base_instance )
	uint drawId = baseInstance + uint( gl_InstanceID );
@end

	@insertpiece( custom_vs_preExecution )

	CellData cellData = instance.cellData[drawId];

	// Map pointInLine from range [0; 12) to range [0; 9] so that it reads:
	// 0 0 1 2 3 4 5 6 7 8 9 9
	uint pointInLine = uint(gl_VertexID) % (cellData.numVertsPerLine.x); // cellData.numVertsPerLine.x = 12
	pointInLine = uint(clamp( int(pointInLine) - 1, 0, int(cellData.numVertsPerLine.x - 3u) ));

	uvec2 uVertexPos;

	uVertexPos.x = pointInLine >> 1u;
	// Even numbers are the next line, odd numbers are current line.
	uVertexPos.y = (pointInLine & 0x01u) == 0u ? 1u : 0u;
	uVertexPos.y += uint(gl_VertexID) / cellData.numVertsPerLine.x;

@property( use_skirts )
	// Apply skirt.
	bool isSkirt =( pointInLine <= 1u ||
					pointInLine >= (cellData.numVertsPerLine.x - 4u) ||
					uVertexPos.y == 0u ||
					uVertexPos.y == (cellData.numVertsPerLine.z + 2u) );

	// Now shift X position for the left & right skirts
	uVertexPos.x = uint( max( int(uVertexPos.x) - 1, 0 ) );
	uVertexPos.x = min( uVertexPos.x, ((cellData.numVertsPerLine.x - 7u) >> 1u) );

	// Now shift Y position for the front & back skirts
	uVertexPos.y = uint( max( int(uVertexPos.y) - 1, 0 ) );
	uVertexPos.y = min( uVertexPos.y, cellData.numVertsPerLine.z );
@end

	uint lodLevel = cellData.numVertsPerLine.y;
	uVertexPos = uVertexPos << lodLevel;

	uVertexPos.xy = uvec2( clamp( ivec2(uVertexPos.xy) + cellData.xzTexPosBounds.xy,
						   ivec2( 0, 0 ), cellData.xzTexPosBounds.zw ) );

	vec3 worldPos;
	worldPos.xz = vec2( uVertexPos.xy ) * cellData.scale.xz + cellData.pos.xz;

	// ===== Ocean wave parameters =====
	float timer = cellData.oceanTime.x;
	float timeScale = cellData.oceanTime.y;
	float freqScale = cellData.oceanTime.z;
	float chaosPacked = cellData.oceanTime.w;
	bool  isUnderwater = chaosPacked < 0.0;
	float chaos = abs(chaosPacked);
	
	outVs.wavesIntensity = timeScale;

	timer *= timeScale;
	float uvScale = uintBitsToFloat( cellData.numVertsPerLine.w );
	uvScale *= freqScale;

	const float TEXTURE_DEPTH = 128.0;

	// Chaos frequencies for wave variation
	float chaosA = sin( timer * 0.00173 ) * chaos;
	float chaosB = sin( timer * 0.00127 + 1.0 ) * chaos;
	float chaosC = sin( timer * 0.00089 + 2.0 ) * chaos;

	float s, c;

	// ===== UV1 - First wave layer =====
	float rot = 0.48 + chaosA * 0.25;
	s = sin(rot); c = cos(rot);
	outVs.uv1.xy = vec2( c * worldPos.x + s * worldPos.z, -s * worldPos.x + c * worldPos.z ) * 0.24 * uvScale;
	float z1 = mod(timer * 0.08, TEXTURE_DEPTH);
	outVs.uv1.z = z1;
	float h1 = textureLod( terrainData, vec3(outVs.uv1.xy, z1), 0.0 ).z;

	// ===== UV2 - Second wave layer =====
	rot = 0.17 + chaosB * 0.23;
	s = sin(rot); c = cos(rot);
	outVs.uv2.xy = vec2( c * worldPos.x + s * worldPos.z, -s * worldPos.x + c * worldPos.z ) * 0.08 * uvScale;
	float z2 = mod(timer * 0.076, TEXTURE_DEPTH);
	outVs.uv2.z = z2;
	float h2 = textureLod( terrainData, vec3(outVs.uv2.xy, z2), 0.0 ).z;

	// ===== UV3 - Third wave layer =====
	rot = 0.09 + chaosC * 0.21;
	s = sin(rot); c = cos(rot);
	outVs.uv3.xy = vec2(c * worldPos.x + s * worldPos.z, -s * worldPos.x + c * worldPos.z ) * 0.17 * uvScale;
	float z3 = mod(timer * 0.069, TEXTURE_DEPTH);
	outVs.uv3.z = z3;
	float h3 = textureLod( terrainData, vec3(outVs.uv3.xy, z3), 0.0 ).z;

	// ===== UV4 - Fourth wave layer =====
	outVs.uv4.xy = worldPos.xz * 0.3 * uvScale;
	float z4 = mod(timer * 0.063, TEXTURE_DEPTH);
	outVs.uv4.z = z4;
	float h4 = textureLod( terrainData, vec3(outVs.uv4.xy, z4), 0.0 ).z;

	// ===== Blend weights for wave layers =====
	outVs.blendWeight = textureLod( blendMap, outVs.uv1.xy * 0.1, 0.0 ).xyz;

	// ===== Calculate final wave height =====
	float height = mix( mix(h1, h2, outVs.blendWeight.x), mix(h3, h4, outVs.blendWeight.z), outVs.blendWeight.y );

	worldPos.y = (height * 3.0 - 1.7) * uvScale;
	worldPos.y = worldPos.y * cellData.scale.y + cellData.pos.y;

@property( use_skirts )
	if( isSkirt && !isUnderwater )
		worldPos.y = cellData.pos.y - 10000.0;
@end

	outVs.waveHeight = pow( height, 6.0 ) * 4.0;

	// ===== UV0 for texture mapping =====
	float invWidth = 1.0 / float(cellData.xzTexPosBounds.z + 1);
	outVs.uv0.xy = vec2( uVertexPos.xy ) * vec2( invWidth, cellData.scale.w );
	
	// ===== World position output =====
	outVs.wpos = worldPos.xyz;

	// ===== Transform to view/clip space =====
	@insertpiece( VertexOceanTransform )

	// ===== Normal (flat ocean surface for now) =====
	vec3 normalWS = vec3(0.0, 1.0, 0.0);
	outVs.normal = normalize( normalWS * mat3(passBuf.view) );

	// ===== Shadows =====
	@insertpiece( DoShadowReceiveVS )

@property( hlms_pssm_splits )	outVs.depth = gl_Position.z;@end

	@insertpiece( custom_vs_posExecution )
}