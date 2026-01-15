@property( false )
@insertpiece( SetCrossPlatformSettings )
@insertpiece( SetCompatibilityLayer )

layout(std140) uniform;
#define FRAG_COLOR		0
layout(location = FRAG_COLOR) out vec4 outColour;

uniform sampler3D terrainData;

in block
{
@insertpiece( Ocean_VStoPS_block )
} inPs;

void main()
{
	outColour = vec4( inPs.uv0.xy, 0.0, 1.0 );
}

@end
@property( !false )
@insertpiece( SetCrossPlatformSettings )
@property( GL3+ < 430 )
	@property( hlms_tex_gather )#extension GL_ARB_texture_gather: require@end
@end

@property( hlms_amd_trinary_minmax )#extension GL_AMD_shader_trinary_minmax: require@end
@insertpiece( SetCompatibilityLayer )

layout(std140) uniform;
#define FRAG_COLOR		0
layout(location = FRAG_COLOR) out vec4 outColour;

@property( hlms_vpos )
in vec4 gl_FragCoord;
@end

// START UNIFORM DECLARATION
@insertpiece( PassDecl )
@insertpiece( OceanMaterialDecl )
@insertpiece( OceanInstanceDecl )
@insertpiece( custom_ps_uniformDeclaration )
// END UNIFORM DECLARATION

in block
{
@insertpiece( Ocean_VStoPS_block )
} inPs;

uniform sampler3D terrainData;
uniform sampler2D blendMap;

@property( hlms_forwardplus )
/*layout(binding = 1) */uniform usamplerBuffer f3dGrid;
/*layout(binding = 2) */uniform samplerBuffer f3dLightList;
@end
@property( num_textures )uniform sampler2DArray textureMaps[@value( num_textures )];@end
@property( envprobe_map )uniform samplerCube	texEnvProbeMap;@end

vec4 diffuseCol;
@insertpiece( FresnelType ) F0;
float ROUGHNESS;

vec3 nNormal;

@property( hlms_lights_spot_textured )@insertpiece( DeclQuat_zAxis )
vec3 qmul( vec4 q, vec3 v )
{
	return v + 2.0 * cross( cross( v, q.xyz ) + q.w * v, q.xyz );
}
@end

@property( detail_maps_normal )vec3 getTSDetailNormal( sampler2DArray normalMap, vec3 uv )
{
	vec3 tsNormal;
@property( signed_int_textures )
	// Normal texture must be in U8V8 or BC5 format!
	tsNormal.xy = texture( normalMap, uv ).xy;
@end @property( !signed_int_textures )
	// Normal texture must be in LA format!
	tsNormal.xy = texture( normalMap, uv ).xw * 2.0 - 1.0;
@end
	tsNormal.z	= sqrt( max( 0.0, 1.0 - tsNormal.x * tsNormal.x - tsNormal.y * tsNormal.y ) );

	return tsNormal;
}
	@foreach( 4, n )
		@property( normal_weight_detail@n )
			@piece( detail@n_nm_weight_mul ) * material.normalWeights.@insertpiece( detail_swizzle@n )@end
		@end
	@end
@end

@insertpiece( DeclareBRDF )

@insertpiece( DeclShadowMapMacros )
@insertpiece( DeclShadowSamplers )
@insertpiece( DeclShadowSamplingFuncs )

void main()
{
	@insertpiece( custom_ps_preExecution )

	@insertpiece( custom_ps_posMaterialLoad )

	// ===== Sample ocean texture data =====
	vec4 textureValue  = textureLod( terrainData, vec3(inPs.uv1.xy, inPs.uv1.z), 0.0 );
	vec4 textureValue2 = textureLod( terrainData, vec3(inPs.uv2.xy, inPs.uv2.z), 0.0 );
	vec4 textureValue3 = textureLod( terrainData, vec3(inPs.uv3.xy, inPs.uv3.z), 0.0 );
	vec4 textureValue4 = textureLod( terrainData, vec3(inPs.uv4.xy, inPs.uv4.z), 0.0 );

	textureValue = mix(textureValue, textureValue2, inPs.blendWeight.x);
	textureValue = mix(textureValue, textureValue3, inPs.blendWeight.y);
	textureValue = mix(textureValue, textureValue4, inPs.blendWeight.z);
	
	// ===== Wave calculations =====
	float waveIntensity = clamp( inPs.wavesIntensity, 0.0, 1.0 );
	float waveFactor    = clamp( inPs.waveHeight, 0.0, 1.0 ) * waveIntensity;
	
	// ===== Foam calculation =====
	float foam = pow( clamp( textureValue.w, 0.0, 1.0 ), 2.0 ) * waveFactor * 0.5;
	foam = clamp( foam, 0.0, 1.0 );
	
	ROUGHNESS = mix( 0.02, 0.3, foam );
	
	// ===== Base water color =====
	diffuseCol.xyz = mix( material.deepColour.xyz, material.shallowColour.xyz, waveFactor * 0.7 );
	vec3 foamColour = vec3( 0.9, 0.9, 0.9 );
	diffuseCol.xyz = mix( diffuseCol.xyz, foamColour, foam );
	diffuseCol.w = 1.0;

	/// Apply the material's diffuse over the textures
	diffuseCol.xyz *= material.kD.xyz;

	// Calculate F0 from metalness, and dim kD as metalness gets bigger.
	float metalness = 0.0; // Water is non-metallic
	F0 = mix( vec3( 0.02 ), @insertpiece( kD ).xyz * 3.14159, metalness );
	@insertpiece( kD ).xyz = @insertpiece( kD ).xyz - @insertpiece( kD ).xyz * metalness;

	// ===== Normal from wave texture =====
	vec3 normalTS = textureValue.xyz * 2.0 - 1.0;
	
	// Build TBN matrix
	vec3 geomNormal = normalize( inPs.normal );
	vec3 viewSpaceUnitX = vec3( passBuf.view[0].x, passBuf.view[1].x, passBuf.view[2].x );
	vec3 vTangent = normalize( cross( geomNormal, viewSpaceUnitX ) );
	vec3 vBinormal = cross( vTangent, geomNormal );
	mat3 TBN = mat3( vBinormal, vTangent, geomNormal );
	
	nNormal = normalize( TBN * normalTS );

	// ===== Shadows =====
	@property( !(hlms_pssm_splits || (!hlms_pssm_splits && hlms_num_shadow_map_lights && hlms_lights_directional)) )
		float fShadow = 1.0;
	@end
	@insertpiece( DoDirectionalShadowMaps )

	// Everything's in Camera space
@property( hlms_lights_spot || ambient_hemisphere || envprobe_map || hlms_forwardplus )
	vec3 viewDir	= normalize( -inPs.pos );
	float NdotV		= clamp( dot( nNormal, viewDir ), 0.0, 1.0 );@end

@property( !ambient_fixed )
	vec3 finalColour = vec3(0);
@end @property( ambient_fixed )
	vec3 finalColour = passBuf.ambientUpperHemi.xyz * @insertpiece( kD ).xyz;
@end

	@insertpiece( custom_ps_preLights )

@property( !custom_disable_directional_lights )
@property( hlms_lights_directional )
	finalColour += BRDF( passBuf.lights[0].position.xyz, viewDir, NdotV, passBuf.lights[0].diffuse, passBuf.lights[0].specular ) @insertpiece(DarkenWithShadowFirstLight);
@end
@foreach( hlms_lights_directional, n, 1 )
	finalColour += BRDF( passBuf.lights[@n].position.xyz, viewDir, NdotV, passBuf.lights[@n].diffuse, passBuf.lights[@n].specular )@insertpiece( DarkenWithShadow );@end
@foreach( hlms_lights_directional_non_caster, n, hlms_lights_directional )
	finalColour += BRDF( passBuf.lights[@n].position.xyz, viewDir, NdotV, passBuf.lights[@n].diffuse, passBuf.lights[@n].specular );@end
@end

@property( hlms_lights_point || hlms_lights_spot )	vec3 lightDir;
	float fDistance;
	vec3 tmpColour;
	float spotCosAngle;@end

	// Point lights
@foreach( hlms_lights_point, n, hlms_lights_directional_non_caster )
	lightDir = passBuf.lights[@n].position.xyz - inPs.pos;
	fDistance= length( lightDir );
	if( fDistance <= passBuf.lights[@n].attenuation.x )
	{
		lightDir *= 1.0 / fDistance;
		tmpColour = BRDF( lightDir, viewDir, NdotV, passBuf.lights[@n].diffuse, passBuf.lights[@n].specular )@insertpiece( DarkenWithShadowPoint );
		float atten = 1.0 / (0.5 + (passBuf.lights[@n].attenuation.y + passBuf.lights[@n].attenuation.z * fDistance) * fDistance );
		finalColour += tmpColour * atten;
	}@end

	// Spot lights
@foreach( hlms_lights_spot, n, hlms_lights_point )
	lightDir = passBuf.lights[@n].position.xyz - inPs.pos;
	fDistance= length( lightDir );
@property( !hlms_lights_spot_textured )	spotCosAngle = dot( normalize( inPs.pos - passBuf.lights[@n].position.xyz ), passBuf.lights[@n].spotDirection );@end
@property( hlms_lights_spot_textured )	spotCosAngle = dot( normalize( inPs.pos - passBuf.lights[@n].position.xyz ), zAxis( passBuf.lights[@n].spotQuaternion ) );@end
	if( fDistance <= passBuf.lights[@n].attenuation.x && spotCosAngle >= passBuf.lights[@n].spotParams.y )
	{
		lightDir *= 1.0 / fDistance;
	@property( hlms_lights_spot_textured )
		vec3 posInLightSpace = qmul( spotQuaternion[@value(spot_params)], inPs.pos );
		float spotAtten = texture( texSpotLight, normalize( posInLightSpace ).xy ).x;
	@end
	@property( !hlms_lights_spot_textured )
		float spotAtten = clamp( (spotCosAngle - passBuf.lights[@n].spotParams.y) * passBuf.lights[@n].spotParams.x, 0.0, 1.0 );
		spotAtten = pow( spotAtten, passBuf.lights[@n].spotParams.z );
	@end
		tmpColour = BRDF( lightDir, viewDir, NdotV, passBuf.lights[@n].diffuse, passBuf.lights[@n].specular )@insertpiece( DarkenWithShadow );
		float atten = 1.0 / (0.5 + (passBuf.lights[@n].attenuation.y + passBuf.lights[@n].attenuation.z * fDistance) * fDistance );
		finalColour += tmpColour * (atten * spotAtten);
	}@end

@insertpiece( forward3dLighting )

@property( envprobe_map || ambient_hemisphere )
	vec3 reflDir = 2.0 * dot( viewDir, nNormal ) * nNormal - viewDir;

	@property( envprobe_map )
		vec3 envColourS = textureLod( texEnvProbeMap, reflDir * passBuf.invViewMatCubemap, ROUGHNESS * 12.0 ).xyz @insertpiece( ApplyEnvMapScale );
		vec3 envColourD = textureLod( texEnvProbeMap, nNormal * passBuf.invViewMatCubemap, 11.0 ).xyz @insertpiece( ApplyEnvMapScale );
		@property( !hw_gamma_read )	// Gamma to linear space
			envColourS = envColourS * envColourS;
			envColourD = envColourD * envColourD;
		@end
	@end
	@property( ambient_hemisphere )
		float ambientWD = dot( passBuf.ambientHemisphereDir.xyz, nNormal ) * 0.5 + 0.5;
		float ambientWS = dot( passBuf.ambientHemisphereDir.xyz, reflDir ) * 0.5 + 0.5;

		@property( envprobe_map )
			envColourS	+= mix( passBuf.ambientLowerHemi.xyz, passBuf.ambientUpperHemi.xyz, ambientWD );
			envColourD	+= mix( passBuf.ambientLowerHemi.xyz, passBuf.ambientUpperHemi.xyz, ambientWS );
		@end @property( !envprobe_map )
			vec3 envColourS = mix( passBuf.ambientLowerHemi.xyz, passBuf.ambientUpperHemi.xyz, ambientWD );
			vec3 envColourD = mix( passBuf.ambientLowerHemi.xyz, passBuf.ambientUpperHemi.xyz, ambientWS );
		@end
	@end

	@insertpiece( BRDF_EnvMap )
@end

@property( !hw_gamma_write )
	// Linear to Gamma space
	outColour.xyz	= sqrt( finalColour );
@end @property( hw_gamma_write )
	outColour.xyz	= finalColour;
@end

@property( hlms_alphablend )
	outColour.w	= 1.0;
@end @property( !hlms_alphablend )
	outColour.w	= 1.0;@end

	@property( debug_pssm_splits )
		outColour.xyz = mix( outColour.xyz, debugPssmSplit.xyz, 0.2 );
	@end
	
	@insertpiece( custom_ps_posExecution )
}
@end