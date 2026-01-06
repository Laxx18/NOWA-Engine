// Ocean material & decl pieces.
// This file exists because Terra's TerraMaterialDecl is Metal-only in modern Ogre-Next.
// Ocean needs its own material data without colliding with PBS' struct Material.

@piece( OceanMaterialStructDecl )
struct OceanMaterial
{
    float4 deepColour;     // xyz = deep colour, w padding
    float4 shallowColour;  // xyz = shallow colour, w = wavesScale
};
@end

@piece( OceanMaterialAccessors )
    #define materialWavesScale (oceanMaterial.shallowColour.w)
@end

@piece( OceanMaterialDecl )
    CONST_BUFFER( OceanMaterialBuf, 1 )
    {
        OceanMaterial oceanMaterial;
    };
@end

//------------------------------------------------------------
// Vertex -> Pixel interface
//------------------------------------------------------------
@piece( Ocean_VStoPS_block )
    float3 pos            : TEXCOORD0;
    float  waveHeight     : TEXCOORD1;
    float  wavesIntensity : TEXCOORD2;
    
    float2 uv0            : TEXCOORD3;
    float3 uv1            : TEXCOORD4;
    float3 uv2            : TEXCOORD5;
    float3 uv3            : TEXCOORD6;
    float3 uv4            : TEXCOORD7;
    
    float3 blendWeight    : TEXCOORD8;

    @property( hlms_num_shadow_map_lights >= 1 )
        @property( !hlms_shadowmap0_is_point_light )
            float4 posL0 : TEXCOORD9;
        @end
    @end
    
    @property( hlms_num_shadow_map_lights >= 2 )
        @property( !hlms_shadowmap1_is_point_light )
            float4 posL1 : TEXCOORD10;
        @end
    @end
    
    @property( hlms_num_shadow_map_lights >= 3 )
        @property( !hlms_shadowmap2_is_point_light )
            float4 posL2 : TEXCOORD11;
        @end
    @end
    
    @property( hlms_pssm_splits )
        float depth : TEXCOORD12;
    @end
    
    float3 wpos : TEXCOORD13;

    float3 normal : TEXCOORD14;
@end


