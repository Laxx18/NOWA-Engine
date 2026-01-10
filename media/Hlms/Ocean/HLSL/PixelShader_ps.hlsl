@insertpiece( SetCrossPlatformSettings )
@insertpiece( SetCompatibilityLayer )

@insertpiece( DefaultTerraHeaderPS )

// ---------------------------------------------------------------------
// SAFETY: objLightMask fallback
// Terra / PBS may reference objLightMask even when fine light masks are off
// ---------------------------------------------------------------------
#ifndef OGRE_HAS_OBJ_LIGHT_MASK
    #define OGRE_HAS_OBJ_LIGHT_MASK
    uint objLightMask = 0xFFFFFFFFu;
#endif

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

Texture3D terrainData;
Texture2D terrainShadows;

SamplerState terrainDataSampler    : register(s0);
SamplerState terrainShadowsSampler : register(s1);

@insertpiece( DeclShadowSamplers )

@property( hlms_forwardplus )
    Buffer<uint>   f3dGrid;
    Buffer<float4> f3dLightList;
@end

@property( envprobe_map )
    TextureCube texEnvProbeMap;

    SamplerState texEnvProbeMapSampler : register( s15 );
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
	
	// -------------------------------------------------------------
	// Wave factors (SAFE)
	// -------------------------------------------------------------
	float waveIntensity = saturate( inPs.wavesIntensity );
	float waveFactor    = saturate( inPs.waveHeight ) * waveIntensity;

	// -------------------------------------------------------------
	// Foam (defined ONCE, bounded)
	// -------------------------------------------------------------
	float foam = pow( saturate( textureValue.w ), 2.0f ) * waveFactor * 0.5f;
	foam = saturate( foam );

	// Roughness: smoother water with less foam influence
	ROUGHNESS = lerp( 0.02f, 0.3f, foam );

	// -------------------------------------------------------------
	// Base water colour (goes into diffuseCol for lighting)
	// -------------------------------------------------------------
	diffuseCol.xyz = lerp( oceanMaterial.deepColour.xyz, oceanMaterial.shallowColour.xyz, waveFactor * 0.7f );

	// Foam replaces colour (NOT additive!)
	float3 foamColour = float3( 0.9f, 0.9f, 0.9f );
	diffuseCol.xyz = lerp( diffuseCol.xyz, foamColour, foam );

	// Alpha (keep as before)
	diffuseCol.w = 1.0f;

    F0 = make_float_fresnel( 0.03f );

    nNormal.xy = textureValue.xy * 2.0f - 1.0f;
    nNormal.xy *= waveFactor;
    float dotXY = dot( nNormal.xy, nNormal.xy );
    nNormal.z = 1.0f; // match GLSL

    @insertpiece( custom_ps_posSampleNormal )

    // -------------------------------------------------------------
	// Pixel-only ocean normal reconstruction (match GLSL exactly)
	// -------------------------------------------------------------

	// Tangent-space normal from texture
	float3 nNormalTS;
	nNormalTS.xy = textureValue.xy * 2.0f - 1.0f;
	nNormalTS.xy *= waveFactor;
	nNormalTS.z  = 1.0f;

	// Geometric normal is always +Y (WORLD)
	float3 geomNormal = float3( 0.0f, 1.0f, 0.0f );

	// Convert geom normal to VIEW space exactly like GLSL: geomNormal * mat3(passBuf.view)
	geomNormal = normalize( mul( geomNormal, (float3x3)passBuf.view ) );

	// GLSL: vec3( view[0].x, view[1].x, view[2].x )  == first ROW of view matrix (x across columns)
	float3 viewSpaceUnitX = float3( passBuf.view._11, passBuf.view._12, passBuf.view._13 );

	// TBN (same cross order as GLSL)
	float3 vTangent  = normalize( cross( geomNormal, viewSpaceUnitX ) );
	float3 vBinormal = cross( vTangent, geomNormal );

	// GLSL: nNormal = normalize( TBN * nNormalTS ); with TBN columns (vBinormal, vTangent, geomNormal)
	// Do the equivalent explicitly (no matrix layout pitfalls):
	nNormal = normalize( vBinormal * nNormalTS.x + vTangent * nNormalTS.y + geomNormal * nNormalTS.z );

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
    float3 viewPos = inPs.pos;
	float3 viewDir = normalize( -viewPos );
	float  NdotV   = saturate( dot( nNormal, viewDir ) );

	PixelData pixelData;

	pixelData.normal   = nNormal;
	pixelData.diffuse  = midf4( diffuseCol );
	pixelData.specular = midf3( 1.0f, 1.0f, 1.0f ); // Full specular for water
	pixelData.F0       = F0;
	pixelData.viewDir  = viewDir;
	pixelData.NdotV    = NdotV;

	// Roughness: smoother for better reflections
	pixelData.perceptualRoughness = midf_c( ROUGHNESS );
	pixelData.roughness = max( midf_c( 0.002f ), pixelData.perceptualRoughness * pixelData.perceptualRoughness );

    // ---------------------------------------------------------------------
	// Ambient (matches how HlmsOcean sets properties in calculateHashForPreCreate)
	// ---------------------------------------------------------------------
	midf3 finalColour = midf3( 0.0f, 0.0f, 0.0f );

	@property( ambient_fixed )
		// Upper hemi only
		finalColour = passBuf.ambientUpperHemi.xyz * @insertpiece( kD ).xyz;
	@end

	@property( ambient_hemisphere )
		// Blend between lower & upper based on normal vs hemisphere direction.
		// nNormal is view-space in your shader, and passBuf.ambientHemisphereDir is also view-space.
		midf hemi = saturate( dot( nNormal, passBuf.ambientHemisphereDir.xyz ) * _h(0.5f) + _h(0.5f) );
		midf3 ambientHemi = lerp( passBuf.ambientLowerHemi.xyz, passBuf.ambientUpperHemi.xyz, hemi );
		finalColour = ambientHemi * @insertpiece( kD ).xyz;
	@end

    @insertpiece( DeclareObjLightMask )
    @insertpiece( custom_ps_preLights )

    // ---------------------------------------------------------------------
    // Directional Lights
    // ---------------------------------------------------------------------
	
	@property( !custom_disable_directional_lights )
		@property( hlms_lights_directional || hlms_lights_directional_non_caster )

			@property( hlms_lights_directional )
				finalColour += BRDF(
					passBuf.lights[0].position.xyz,
					passBuf.lights[0].diffuse.xyz,
					passBuf.lights[0].specular.xyz,
					pixelData PASSBUF_ARG
				) @insertpiece( DarkenWithShadowFirstLight );
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
	// Env probe / IBL
	// ---------------------------------------------------------------------
    @property( envprobe_map )
		// Reflection is the PRIMARY color source for water!
		float3 reflDirView  = reflect( -viewDir, nNormal );
		
		float3x3 invViewRot = transpose( (float3x3)passBuf.view );
		float3 reflDirWorld = normalize( mul( reflDirView, invViewRot ) );
		
		// Sample environment with roughness-based mip level
		float reflectionLod = pixelData.perceptualRoughness * 7.0f; // Adjust based on your cubemap mips
		float3 envCol = texEnvProbeMap.SampleLevel( texEnvProbeMapSampler, reflDirWorld, reflectionLod ).xyz;
		
		// Fresnel for reflections (water reflects more at grazing angles)
		float oneMinusNdotV = 1.0f - NdotV;
		float fresnelTerm   = oneMinusNdotV * oneMinusNdotV * oneMinusNdotV * oneMinusNdotV * oneMinusNdotV;
		
		// Water has strong fresnel effect
		float fresnelFactor = lerp( 0.02f, 1.0f, fresnelTerm );
		
		// Reflections are the MAIN color for water
		finalColour += envCol * fresnelFactor * 0.8f; // Strong reflection contribution
	@end
	
    // ---------------------------------------------------------------------
    // Output
    // ---------------------------------------------------------------------
                
	finalColour = saturate( finalColour );
				
    float3 outRgb;

	@property( atmosky_npr )
		@property( hlms_fog )
			const float3 cameraPos = float3(
				atmoSettings.skyLightAbsorption.w,
				atmoSettings.sunAbsorption.w,
				atmoSettings.cameraDisplacement.w );

			const float distToCamera = length( inPs.wpos - cameraPos );

			// exp2 fog (matches AtmosphereNpr's style)
			const float transmittance = exp2( -atmoSettings.fogDensity * distToCamera );
			float fogAmount = saturate( 1.0f - transmittance );

			// Optional "fog break"
			const float brightness = max( finalColour.x, max( finalColour.y, finalColour.z ) );
			const float breakFactor =
				saturate( (brightness - atmoSettings.fogBreakMinBrightness) * atmoSettings.fogBreakFalloff );
			fogAmount *= (1.0f - breakFactor);

			finalColour = lerp( finalColour, inPs.fog.xyz, fogAmount );
		@end
	@end

	@property( !hw_gamma_write )
		outRgb = sqrt( finalColour );
	@end
	@property( hw_gamma_write )
		outRgb = finalColour;
	@end

	outPs.colour0.xyz = outRgb;
		
		@property( hlms_alphablend )
		@property( use_texture_alpha )
			outPs.colour0.w = diffuseCol.w;
		@end
		@property( !use_texture_alpha )
			outPs.colour0.w = 1.0f;
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