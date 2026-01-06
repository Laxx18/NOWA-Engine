@insertpiece( SetCrossPlatformSettings )
@insertpiece( SetCompatibilityLayer )

@insertpiece( DefaultTerraHeaderPS )

@insertpiece( OceanMaterialStructDecl )
@insertpiece( OceanMaterialDecl )

@insertpiece( DeclReverseDepthMacros )
@insertpiece( DeclareUvModifierMacros )

// -----------------------------------------------------------------------------
// PBS helper fallbacks (some Ogre-Next versions only define these in PBS shaders)
// -----------------------------------------------------------------------------
#ifndef float_fresnel
    // Default to RGB Fresnel (dielectric).
    #define float_fresnel midf3
    #define float_fresnel_c( x ) midf3_c( x )
    #define make_float_fresnel( x ) midf3_c( x, x, x )
#endif

// -----------------------------------------------------------------------------
// Input: Note: Important, VertexShader has this same struct as OUTPUT, so that  inPs.pos has valid values!
// -----------------------------------------------------------------------------
struct Ocean_VSPS
{
    @insertpiece( Ocean_VStoPS_block )
    float4 gl_Position : SV_Position;
};

// -----------------------------------------------------------------------------
// Output
// -----------------------------------------------------------------------------
struct PS_OUTPUT
{
    float4 colour0 : SV_Target0;
};

// -------------------------------------------------------------------------
// Textures
// -------------------------------------------------------------------------

Texture3D terrainData    : register(t0);
Texture2D terrainShadows   : register(t1);

SamplerState terrainDataSampler    : register(s0);
SamplerState terrainShadowsSampler : register(s1);

@insertpiece( DeclShadowSamplers )

@property( hlms_forwardplus )
    Buffer<uint>   f3dGrid;
    Buffer<float4> f3dLightList;
@end

@property( envprobe_map )
    TextureCube texEnvProbeMap : register(t70);
    SamplerState texEnvProbeMapSampler : register(s70);
@end

@property( hlms_lights_spot_textured )
    @insertpiece( DeclQuat_zAxis )
    float3 qmul( float4 q, float3 v )
    {
        return v + 2.0f * cross( cross( v, q.xyz ) + q.w * v, q.xyz );
    }
@end

// -------------------------------------------------------------------------
// Shadow mapping (PBS pieces)
// -------------------------------------------------------------------------

@insertpiece( DeclShadowMapMacros )
@insertpiece( DeclShadowSamplingFuncs )

#ifndef BRDF
INLINE midf3 BRDF(
    midf3 lightDir,
    midf3 lightDiffuse,
    midf3 lightSpecular,
    PixelData pixelData PASSBUF_ARG_DECL )
{
    // Ogre convention: lightDir points FROM light -> surface
    midf3 L = normalize( -lightDir );
    midf3 N = pixelData.normal;

    midf NdotL = saturate( dot( N, L ) );

    // Simple Lambert + reduced specular to avoid bright spots
    midf3 diffuse  = pixelData.diffuse.xyz * lightDiffuse * NdotL;
    midf3 specular = lightSpecular * pow( NdotL, _h(32.0f) ) * _h(0.2);

    return diffuse + specular * pixelData.F0;
}
#endif

// -------------------------------------------------------------------------
// Pixel Shader
// -------------------------------------------------------------------------
PS_OUTPUT main(
Ocean_VSPS inPs
@property( hlms_vpos )
    , float4 gl_FragCoord : SV_Position
@end
@property( two_sided_lighting )
    , bool gl_FrontFacing : SV_IsFrontFace
@end
)
{
    PS_OUTPUT outPs;
    @insertpiece( custom_ps_preExecution )
    
    @insertpiece( custom_ps_posMaterialLoad )
    
    float4 diffuseCol;
    float_fresnel F0;
    float ROUGHNESS;
    float3 nNormal;

    float4 textureValue  = terrainData.SampleLevel(terrainDataSampler, float3(inPs.uv1.xy, inPs.uv1.z), 0);
    float4 textureValue2 = terrainData.SampleLevel(terrainDataSampler, float3(inPs.uv2.xy, inPs.uv2.z), 0);
    float4 textureValue3 = terrainData.SampleLevel(terrainDataSampler, float3(inPs.uv3.xy, inPs.uv3.z), 0);
    float4 textureValue4 = terrainData.SampleLevel(terrainDataSampler, float3(inPs.uv4.xy, inPs.uv4.z), 0);

    textureValue = lerp(textureValue, textureValue2, inPs.blendWeight.x);
    textureValue = lerp(textureValue, textureValue3, inPs.blendWeight.y);
    textureValue = lerp(textureValue, textureValue4, inPs.blendWeight.z);

    float waveIntensity = inPs.wavesIntensity;

    float foam = pow( textureValue.w, 3.0f - waveIntensity ) * 0.5f * waveIntensity * waveIntensity;

    ROUGHNESS = 0.02f + foam;

    diffuseCol.xyz = lerp( oceanMaterial.deepColour.xyz, oceanMaterial.shallowColour.xyz, inPs.waveHeight * waveIntensity );

    diffuseCol.w = 1.0f;
    // Reduce foam contribution to avoid bright spots
    diffuseCol.xyz += foam * 0.5;

    F0 = make_float_fresnel( 0.03f );

    nNormal.xy = textureValue.xy * 2.0f - 1.0f;
    nNormal.xy *= waveIntensity;
    float dotXY = dot( nNormal.xy, nNormal.xy );
    nNormal.z = 1.0f; // match GLSL

    @insertpiece( custom_ps_posSampleNormal )

    // -------------------------------------------------------------
    // Pixel-only ocean normal reconstruction
    // -------------------------------------------------------------

    // Tangent-space normal from texture
    float3 nNormalTS;
    nNormalTS.xy = textureValue.xy * 2.0f - 1.0f;
    nNormalTS.xy *= waveIntensity;
    nNormalTS.z  = 1.0f;

    // Geometric normal (WORLD)
    float3 geomNormalWS = float3( 0.0f, 1.0f, 0.0f );

    // Convert to VIEW space
    float3 geomNormalVS = normalize( mul( geomNormalWS, (float3x3)passBuf.view ) );

    // Camera X axis in VIEW space
    float3 viewSpaceUnitX = float3(passBuf.view._11, passBuf.view._21, passBuf.view._31 );

    // Build stable TBN in VIEW space
    float3 vTangent  = normalize( cross( geomNormalVS, viewSpaceUnitX ) );
    float3 vBinormal = cross( vTangent, geomNormalVS );

    // Order: (binormal, tangent, normal)
    float3x3 TBN = float3x3(vBinormal, vTangent, geomNormalVS );

    // Final view-space normal
    nNormal = normalize( mul( nNormalTS, TBN ) ); // match GLSL column semantics

    // ---------------------------------------------------------------------
    // Shadows
    // ---------------------------------------------------------------------

    @property( terra_enabled )
        // TODO: Is this required for terra shadows?
        float fTerrainShadow = terrainShadows.Sample( terrainShadowsSampler, inPs.uv0.xy ).x;
    @end

    // If there are no directional shadow maps at all, PBS' DoDirectionalShadowMaps doesn't declare fShadow.
    // Provide a default so the rest of the shader can always multiply by it.
    @property( !(hlms_pssm_splits || (!hlms_pssm_splits && hlms_num_shadow_map_lights && hlms_lights_directional)) )
       midf fShadow = _h( 1.0 );
    @end

    @insertpiece(DoDirectionalShadowMaps)
    @property( terra_enabled )
        fShadow *= fTerrainShadow;
    @end

    // ---------------------------------------------------------------------
    // Lighting
    // ---------------------------------------------------------------------

    // -------------------------------------------------------------
    // View direction & PixelData (ALWAYS VALID)
    // -------------------------------------------------------------
    float3 viewPos = inPs.pos; // already in view space
    float3 viewDir = normalize( -viewPos );
    float  NdotV   = clamp( dot( nNormal, viewDir ), 0.0f, 1.0f );

    PixelData pixelData;

    // core data
    pixelData.normal   = nNormal;
    pixelData.diffuse  = midf4( diffuseCol );
    pixelData.specular = midf3( 1.0f, 1.0f, 1.0f );
    pixelData.F0       = F0;

    // roughness handling
    @property( hlms_forwardplus )
        pixelData.perceptualRoughness = midf_c( 0.15f );
    @end
    @property( !hlms_forwardplus )
        pixelData.perceptualRoughness = ROUGHNESS;
    @end

    pixelData.roughness = max( midf_c( 0.002f ), pixelData.perceptualRoughness * pixelData.perceptualRoughness );

    midf3 finalColour = diffuseCol.xyz * _h(0.1f);

    @property( !ambient_fixed )
        finalColour += pixelData.diffuse.xyz * passBuf.ambientUpperHemi.xyz;
    @end
    @property( ambient_fixed )
        finalColour = passBuf.ambientUpperHemi.xyz * @insertpiece( kD ).xyz;
    @end

    @insertpiece( DeclareObjLightMask )
    @insertpiece( custom_ps_preLights )

    // ---------------------------------------------------------------------
    // Directional Lights
    // ---------------------------------------------------------------------

    @property( hlms_lights_directional || hlms_lights_directional_non_caster )

        @property( hlms_lights_directional )
            finalColour += BRDF(
                passBuf.lights[0].position.xyz,
                passBuf.lights[0].diffuse.xyz,
                passBuf.lights[0].specular.xyz,
                pixelData PASSBUF_ARG
            ) * fShadow;
        @end

        @foreach( hlms_lights_directional, n, 1 )
            finalColour += BRDF(
                passBuf.lights[@n].position.xyz,
                passBuf.lights[@n].diffuse.xyz,
                passBuf.lights[@n].specular.xyz,
                pixelData PASSBUF_ARG
            ) @insertpiece( DarkenWithShadow );
        @end

        @foreach( hlms_lights_directional_non_caster, n, hlms_lights_directional )
            finalColour += BRDF(
                passBuf.lights[@n].position.xyz,
                passBuf.lights[@n].diffuse.xyz,
                passBuf.lights[@n].specular.xyz,
                pixelData PASSBUF_ARG
            );
        @end
    @end

    // ---------------------------------------------------------------------
    // Point and Spot Lights
    // ---------------------------------------------------------------------
    
    @property( hlms_lights_point || hlms_lights_spot )
        float3 lightDir;
        float fDistance;
        midf3 tmpColour;
        float spotCosAngle;
    @end

    // Point lights
    @foreach( hlms_lights_point, n, hlms_lights_directional_non_caster )
        lightDir = passBuf.lights[@n].position - inPs.pos;
        fDistance = length( lightDir );
        if( fDistance <= passBuf.lights[@n].attenuation.x )
        {
            lightDir *= 1.0f / fDistance;
            tmpColour = BRDF(
                lightDir,
                passBuf.lights[@n].diffuse.xyz,
                passBuf.lights[@n].specular.xyz,
                pixelData PASSBUF_ARG
            ) @insertpiece( DarkenWithShadowPoint );
            float atten = 1.0f / (0.5f + (passBuf.lights[@n].attenuation.y + passBuf.lights[@n].attenuation.z * fDistance) * fDistance );
            finalColour += tmpColour * atten;
        }
    @end

    // Spot lights
    @foreach( hlms_lights_spot, n, hlms_lights_point )
    
        lightDir = passBuf.lights[@n].position - inPs.pos;
        fDistance = length( lightDir );
        
        @property( !hlms_lights_spot_textured )
            spotCosAngle = dot( normalize( inPs.pos - passBuf.lights[@n].position ), passBuf.lights[@n].spotDirection );
        @end
        
        @property( hlms_lights_spot_textured )
            spotCosAngle = dot( normalize( inPs.pos - passBuf.lights[@n].position ), zAxis( passBuf.lights[@n].spotQuaternion ) );
        @end
        
        if( fDistance <= passBuf.lights[@n].attenuation.x && spotCosAngle >= passBuf.lights[@n].spotParams.y )
        {
            lightDir *= 1.0f / fDistance;
            
            @property( hlms_lights_spot_textured )
                float3 posInLightSpace = qmul( spotQuaternion[@value(spot_params)], inPs.pos );
                float spotAtten = texSpotLight.Sample( samplerState[@value(spot_params)], normalize( posInLightSpace ).xy ).x;
            @end
            
            @property( !hlms_lights_spot_textured )
                float spotAtten = saturate( (spotCosAngle - passBuf.lights[@n].spotParams.y) * passBuf.lights[@n].spotParams.x );
                spotAtten = pow( spotAtten, passBuf.lights[@n].spotParams.z );
            @end
            
            tmpColour = BRDF(
                lightDir,
                passBuf.lights[@n].diffuse.xyz,
                passBuf.lights[@n].specular.xyz,
                pixelData PASSBUF_ARG
            ) @insertpiece( DarkenWithShadow );
            
            float atten = 1.0f / (0.5f + (passBuf.lights[@n].attenuation.y + passBuf.lights[@n].attenuation.z * fDistance) * fDistance );
            finalColour += tmpColour * (atten * spotAtten);
        }
    @end

    @insertpiece( forward3dLighting )

    // ---------------------------------------------------------------------
    // Output
    // ---------------------------------------------------------------------
                
    @property( !hw_gamma_write )
        float3 outRgb = sqrt( finalColour );
    @end
    @property( hw_gamma_write )
        float3 outRgb = finalColour;
    @end

    outPs.colour0.xyz = outRgb;
    
    @property( hlms_alphablend )
        @property( use_texture_alpha )
            outPs.colour0.w = oceanMaterial.F0.w * diffuseCol.w;
        @end
        @property( !use_texture_alpha )
            outPs.colour0.w = oceanMaterial.F0.w;
        @end
    @end
    @property( !hlms_alphablend )
        outPs.colour0.w = 1.0f;
    @end

    @property( debug_pssm_splits )
        outPs.colour0.xyz = lerp( outPs.colour0.xyz, debugPssmSplit.xyz, 0.2f );
    @end
    
    @insertpiece( custom_ps_posExecution )
    
    return outPs;
}