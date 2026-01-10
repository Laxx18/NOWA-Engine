@insertpiece( SetCrossPlatformSettings )
@insertpiece( SetCompatibilityLayer )
@insertpiece( DefaultTerraHeaderVS )

// -----------------------------------------------------------------------------
// Input
// -----------------------------------------------------------------------------
struct VS_INPUT
{
    uint vertexId : SV_VertexID;
    uint drawId   : DRAWID;
    @insertpiece( custom_vs_attributes )
};

// -----------------------------------------------------------------------------
// Output
// -----------------------------------------------------------------------------
struct Ocean_VSPS
{
    @insertpiece( Ocean_VStoPS_block )
    float4 gl_Position : SV_Position;
};

// -------------------------------------------------------------------------
// Textures
// -------------------------------------------------------------------------

Texture3D terrainData;
Texture2D blendMap;

SamplerState samplerState0 : register(s0);
SamplerState samplerState1 : register(s1);

@insertpiece( custom_vs_uniformDeclaration )

// -----------------------------------------------------------------------------
// Vertex Shader
// -----------------------------------------------------------------------------
Ocean_VSPS main( VS_INPUT input )
{
    Ocean_VSPS outVs = (Ocean_VSPS)0;

    @insertpiece( custom_vs_preExecution )

    CellData cellData = cellDataArray[input.drawId];

    uint pointInLine = input.vertexId % cellData.numVertsPerLine.x;
    pointInLine = (uint)clamp( (int)pointInLine - 1, 0, (int)(cellData.numVertsPerLine.x - 3u) );

    uint2 uVertexPos;
    uVertexPos.x = pointInLine >> 1u;
    uVertexPos.y = (pointInLine & 0x01u) == 0u ? 1u : 0u;
    uVertexPos.y += input.vertexId / cellData.numVertsPerLine.x;

    @property( use_skirts )
        bool isSkirt = pointInLine <= 1u || pointInLine >= (cellData.numVertsPerLine.x - 4u) || uVertexPos.y == 0u || uVertexPos.y == (cellData.numVertsPerLine.z + 2u);

        uVertexPos.x = (uint)max( (int)uVertexPos.x - 1, 0 );
        uVertexPos.x = min( uVertexPos.x, ((cellData.numVertsPerLine.x - 7u) >> 1u) );

        uVertexPos.y = (uint)max( (int)uVertexPos.y - 1, 0 );
        uVertexPos.y = min( uVertexPos.y, cellData.numVertsPerLine.z );
    @end

    uint lodLevel = cellData.numVertsPerLine.y;
    uVertexPos <<= lodLevel;

    uVertexPos = (uint2)clamp( (int2)uVertexPos + cellData.xzTexPosBounds.xy, int2(0,0), cellData.xzTexPosBounds.zw );

    float3 worldPos;
    worldPos.xz = (float2)uVertexPos * cellData.scale.xz + cellData.pos.xz;


    float timer = cellData.oceanTime.x;
    float timeScale = cellData.oceanTime.y;
    float freqScale = cellData.oceanTime.z;
    float chaosPacked = cellData.oceanTime.w;
    bool  isUnderwater = chaosPacked < 0.0f;
    float chaos = abs(chaosPacked);
	
	// outVs.wavesIntensity = cellData.scale.y;
	outVs.wavesIntensity = timeScale;

    // ----------------------------------------------------
    // CONTINUOUS TIME
    // ----------------------------------------------------
    timer *= timeScale;

    // ----------------------------------------------------
    // UV scale
    // ----------------------------------------------------
    float uvScale = asfloat( cellData.numVertsPerLine.w );
    uvScale *= freqScale;

    // ----------------------------------------------------
    // Texture depth constant (DECLARE ONLY ONCE!)
    // ----------------------------------------------------
    const float TEXTURE_DEPTH = 128.0f;

    // ----------------------------------------------------
    // Chaos - slower frequencies
    // ----------------------------------------------------
    float chaosA = sin( timer * 0.00173f ) * chaos;
    float chaosB = sin( timer * 0.00127f + 1.0f ) * chaos;
    float chaosC = sin( timer * 0.00089f + 2.0f ) * chaos;
    float chaosD = sin( timer * 0.00061f + 3.0f ) * chaos;

    // ----------------------------------------------------
    // UV generation
    // ----------------------------------------------------
    float s, c;

    // UV1
    float rot = 0.48f + chaosA * 0.25f;
    s = sin(rot); c = cos(rot);
    outVs.uv1.xy = float2( c * worldPos.x + s * worldPos.z, -s * worldPos.x + c * worldPos.z ) * 0.24f * uvScale;
    float z1 = fmod(timer * 0.08f, TEXTURE_DEPTH);
    outVs.uv1.z = z1;
    float h1 = terrainData.SampleLevel( samplerState0, float3(outVs.uv1.xy, z1), 0 ).z;

    // UV2
    rot = 0.17f + chaosB * 0.23f;
    s = sin(rot); c = cos(rot);
    outVs.uv2.xy = float2( c * worldPos.x + s * worldPos.z, -s * worldPos.x + c * worldPos.z ) * 0.08f * uvScale;
    float z2 = fmod(timer * 0.076f, TEXTURE_DEPTH);
    outVs.uv2.z = z2;
    float h2 = terrainData.SampleLevel( samplerState0, float3(outVs.uv2.xy, z2), 0 ).z;

    // UV3
    rot = 0.09f + chaosC * 0.21f;
    s = sin(rot); c = cos(rot);
    outVs.uv3.xy = float2(c * worldPos.x + s * worldPos.z, -s * worldPos.x + c * worldPos.z ) * 0.17f * uvScale;
    float z3 = fmod(timer * 0.069f, TEXTURE_DEPTH);
    outVs.uv3.z = z3;
    float h3 = terrainData.SampleLevel( samplerState0, float3(outVs.uv3.xy, z3), 0 ).z;

    // UV4
    outVs.uv4.xy = worldPos.xz * 0.3f * uvScale;
    float z4 = fmod(timer * 0.063f, TEXTURE_DEPTH);
    outVs.uv4.z = z4;
    float h4 = terrainData.SampleLevel( samplerState0, float3(outVs.uv4.xy, z4), 0 ).z;

    outVs.blendWeight = blendMap.SampleLevel( samplerState1, outVs.uv1.xy * 0.1f, 0 ).xyz;

    float height = lerp( lerp(h1, h2, outVs.blendWeight.x), lerp(h3, h4, outVs.blendWeight.z), outVs.blendWeight.y );

    worldPos.y = (height * 3.0f - 1.7f) * uvScale;
    worldPos.y = worldPos.y * cellData.scale.y + cellData.pos.y;

    @property( use_skirts )
        if( isSkirt && !isUnderwater )
            worldPos.y = cellData.pos.y - 10000.0f;
    @end

    outVs.waveHeight = pow( height, 6.0f ) * 4.0f;

    float invWidth = rcp( (float)(cellData.xzTexPosBounds.z + 1) );
    outVs.uv0.xy = (float2)uVertexPos * float2( invWidth, cellData.scale.w );
    
    @insertpiece( ShadowReceive )

    @foreach( hlms_num_shadow_map_lights, n )
        @property( !hlms_shadowmap@n_is_point_light )
            outVs.posL@n.z = outVs.posL@n.z * passBuf.shadowRcv[@n].shadowDepthRange.y * 0.5f + 0.5f;
        @end
    @end
    
    outVs.pos = mul( float4(worldPos.xyz, 1.0f), passBuf.view ).xyz;
    outVs.gl_Position = mul( float4(worldPos.xyz, 1.0f), passBuf.viewProj );

    @property( hlms_pssm_splits )
        outVs.depth = outVs.gl_Position.z;
    @end

	// TODO: Normals required? Maybe other shader need them? See also Structs_piece_vs_piece_ps.hlsl.
    // float3 normalWS = float3(0.0f, 1.0f, 0.0f);
    // outVs.normal = normalize( mul( float4(normalWS, 0.0f), passBuf.view ).xyz );
    // outVs.wpos = worldPos.xyz;
	
	@property( atmosky_npr )
		@insertpiece( DoAtmosphereNprSky )
	@end

    @insertpiece( custom_vs_posExecution )

    return outVs;
}