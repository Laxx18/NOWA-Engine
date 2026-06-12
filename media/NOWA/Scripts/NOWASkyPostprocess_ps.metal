#include <metal_stdlib>
using namespace metal;

struct PS_INPUT
{
	float3 cameraDir;
};

struct Params
{
	// Scales the LDR cubemap into the HDR luminance range of the scene.
	float hdrSkyPower;
};

fragment float4 main_metal
(
	PS_INPUT inPs [[stage_in]],
	texturecube<float> skyCubemap [[texture(0)]],
	sampler samplerState [[sampler(0)]],
	constant Params &p [[buffer(PARAMETER_SLOT)]]
)
{
	//Cubemaps are left-handed
	return float4( skyCubemap.sample( samplerState, float3( inPs.cameraDir.xy, -inPs.cameraDir.z ) ).xyz * p.hdrSkyPower, 1.0f );
}