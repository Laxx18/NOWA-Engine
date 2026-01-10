// Ocean structs/pieces for NOWA-Engine Ogre-Next HLMS Ocean.
//
// IMPORTANT:
// We DO NOT declare a separate OceanMaterialBuf cbuffer because slot b1 is already used by
// Terra/PBS MaterialBuf. Declaring another cbuffer at b1 causes a collision.
//
// Instead, we extend Terra's Material struct via @insertpiece( custom_materialBuffer )
// (declared below), so Ocean material parameters live inside MaterialBuf.

// ---------------------------------------------------------------------
// Extend Terra/PBS Material struct (MaterialBuf @ slot 1)
// ---------------------------------------------------------------------
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
@pset( texcoord, 0 )

@piece( Ocean_VStoPS_block )
    // Ocean-specific data ONLY
    INTERPOLANT( float3 pos,            @counter(texcoord) );
    INTERPOLANT( float  waveHeight,     @counter(texcoord) );
    INTERPOLANT( float  wavesIntensity, @counter(texcoord) );

    INTERPOLANT( float2 uv0,            @counter(texcoord) );
    INTERPOLANT( float3 uv1,            @counter(texcoord) );
    INTERPOLANT( float3 uv2,            @counter(texcoord) );
    INTERPOLANT( float3 uv3,            @counter(texcoord) );
    INTERPOLANT( float3 uv4,            @counter(texcoord) );

    INTERPOLANT( float3 blendWeight,    @counter(texcoord) );
    INTERPOLANT( float3 wpos,           @counter(texcoord) );
	
	// TODO: Normals required? Maybe other shader need them? See also VertexShader.hlsl.
	// INTERPOLANT( float3 normal,         @counter(texcoord) );
	

    // Let HLMS/Terra own shadows, fog, etc.
    @insertpiece( VStoPS_block )
@end







