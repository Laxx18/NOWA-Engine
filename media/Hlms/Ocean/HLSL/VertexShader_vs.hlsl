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
// Output: Note: Important, PixelShader has this same struct as INPUT, so that inPs.pos has valid values in PixelShader!
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

    outVs.wavesIntensity = cellData.scale.y;

    float timer   = cellData.pos.w;
    float uvScale = asfloat( cellData.numVertsPerLine.w );

    // --- UV generation ---
    float s, c;

    float rot = 0.48f;
    s = sin(rot); c = cos(rot);
    outVs.uv1.xy = float2( c * worldPos.x + s * worldPos.z, -s * worldPos.x + c * worldPos.z ) * 0.24f * uvScale;
    outVs.uv1.z = timer * 0.08f;

    rot = 0.17f;
    s = sin(rot); c = cos(rot);
    outVs.uv2.xy = float2( c * worldPos.x + s * worldPos.z, -s * worldPos.x + c * worldPos.z ) * 0.08f * uvScale;
    outVs.uv2.z = timer * 0.076f;

    rot = 0.09f;
    s = sin(rot); c = cos(rot);
    outVs.uv3.xy = float2( c * worldPos.x + s * worldPos.z, -s * worldPos.x + c * worldPos.z ) * 0.17f * uvScale;
    outVs.uv3.z = timer * 0.069f;

    outVs.uv4.xy = worldPos.xz * 0.3f * uvScale;
    outVs.uv4.z  = timer * 0.063f;

    outVs.blendWeight = blendMap.SampleLevel( samplerState1, outVs.uv1.xy * 0.1f, 0 ).xyz;

    float h1 = terrainData.SampleLevel( samplerState0, float3(outVs.uv1.xy, outVs.uv1.z), 0 ).z;
    float h2 = terrainData.SampleLevel( samplerState0, float3(outVs.uv2.xy, outVs.uv2.z), 0 ).z;
    float h3 = terrainData.SampleLevel( samplerState0, float3(outVs.uv3.xy, outVs.uv3.z), 0 ).z;
    float h4 = terrainData.SampleLevel( samplerState0, float3(outVs.uv4.xy, outVs.uv4.z), 0 ).z;

    float height = lerp( lerp(h1, h2, outVs.blendWeight.x), lerp(h3, h4, outVs.blendWeight.z), outVs.blendWeight.y );

    worldPos.y = (height * 3.0f - 1.7f) * uvScale;

    @property( use_skirts )
        worldPos.y = isSkirt ? 0.001f : worldPos.y;
    @end

    worldPos.y = worldPos.y * cellData.scale.y + cellData.pos.y;

    outVs.waveHeight = pow( height, 6.0f ) * 4.0f;

    float invWidth = rcp( (float)(cellData.xzTexPosBounds.z + 1) );
    outVs.uv0.xy = (float2)uVertexPos * float2( invWidth, cellData.scale.w );
    
    @insertpiece( ShadowReceive )

    @foreach( hlms_num_shadow_map_lights, n )
        @property( !hlms_shadowmap@n_is_point_light )
            outVs.posL@n.z = outVs.posL@n.z * passBuf.shadowRcv[@n].shadowDepthRange.y * 0.5f + 0.5f;
        @end
    @end
    
    // View-space position (used by pixel shader)
    outVs.pos = mul( float4(worldPos.xyz, 1.0f), passBuf.view ).xyz;

    // Clip-space position
    outVs.gl_Position = mul( float4(worldPos.xyz, 1.0f), passBuf.viewProj );

    @property( hlms_pssm_splits )
        outVs.depth = outVs.gl_Position.z;
    @end

    // World-space up normal -> view space
    float3 normalWS = float3(0.0f, 1.0f, 0.0f);
    outVs.normal = normalize( mul( float4(normalWS, 0.0f), passBuf.view ).xyz );

    // World-space position
    outVs.wpos = worldPos.xyz;

    @insertpiece( custom_vs_posExecution )
    
    outVs.pos = float3(10.0f, 0.0f, 0.0f);

    return outVs;
}
