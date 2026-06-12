struct PS_INPUT
{
	float3 cameraDir	: TEXCOORD0;
};

TextureCube<float3> skyCubemap	: register(t0);
SamplerState samplerState		: register(s0);

// Scales the LDR cubemap into the HDR luminance range of the scene.
// Driven by HdrEffectComponent per preset: ~60 for bright sunny day,
// far below 1 for night presets. Default 1.0 = LDR behaviour unchanged.
float hdrSkyPower;

float3 main
(
	PS_INPUT inPs
) : SV_Target0
{
	//Cubemaps are left-handed
	return skyCubemap.Sample( samplerState, float3( inPs.cameraDir.xy, -inPs.cameraDir.z ) ).xyz * hdrSkyPower;
}