@insertpiece( SetCrossPlatformSettings )
@insertpiece( SetCompatibilityLayer )
@insertpiece( DeclareUvModifierMacros )

@insertpiece( DefaultOceanHeaderPS )

// START UNIFORM DECLARATION
@insertpiece( custom_ps_uniformDeclaration )
// END UNIFORM DECLARATION

@insertpiece( PccManualProbeDecl )

struct PS_INPUT
{
    @insertpiece( Ocean_VStoPS_block )
};

@pset( currSampler, samplerStateStart )

@property( !hlms_shadowcaster )

@property( !hlms_render_depth_only )
    @property( hlms_gen_normals_gbuffer )
        #define outPs_normals outPs.normals
    @end
    @property( hlms_prepass )
        #define outPs_shadowRoughness outPs.shadowRoughness
    @end
@end

// Textures
@property( hlms_forwardplus )
    Buffer<uint> f3dGrid : register(t@value(f3dGrid));
    ReadOnlyBuffer( @value(f3dLightList), float4, f3dLightList );
@end

Texture3D terrainData : register(t@value(terrainData));
Texture2D blendMap    : register(t@value(blendMap));

SamplerState terrainDataSampler : register(s@value(terrainData));
SamplerState blendMapSampler    : register(s@value(blendMap));

@foreach( num_samplers, n )
    SamplerState samplerState@value(currSampler) : register(s@counter(currSampler));
@end

@insertpiece( DeclShadowMapMacros )
@insertpiece( DeclShadowSamplers )
@insertpiece( DeclShadowSamplingFuncs )

@insertpiece( DeclOutputType )
@insertpiece( custom_ps_functions )

@insertpiece( output_type ) main
(
    PS_INPUT inPs
    @property( hlms_vpos ), float4 gl_FragCoord : SV_Position@end
    @property( two_sided_lighting ), bool gl_FrontFacing : SV_IsFrontFace@end
)
{
    PS_OUTPUT outPs;
    @insertpiece( custom_ps_preExecution )
    @insertpiece( DefaultOceanBodyPS )
    @insertpiece( custom_ps_posExecution )

@property( !hlms_render_depth_only )
    return outPs;
@end
}
@else ///!hlms_shadowcaster

@insertpiece( DeclOutputType )

@insertpiece( output_type ) main( PS_INPUT inPs )
{
@property( !hlms_render_depth_only || exponential_shadow_maps || hlms_shadowcaster_point )
    PS_OUTPUT outPs;
@end

    @insertpiece( custom_ps_preExecution )
    @insertpiece( DefaultBodyPS )
    @insertpiece( custom_ps_posExecution )

@property( !hlms_render_depth_only || exponential_shadow_maps || hlms_shadowcaster_point )
    return outPs;
@end
}
@end