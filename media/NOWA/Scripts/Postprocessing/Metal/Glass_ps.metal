#include <metal_stdlib>
using namespace metal;

struct PS_INPUT
{
	float2 uv0;
};

struct Params
{
	float GlassWeight;
};

fragment float4 main_metal
(
	PS_INPUT inPs [[stage_in]],
	texture2d<float>	RT				[[texture(0)]],
	texture2d<float>	NormalMap		[[texture(1)]],
	sampler				samplerState0	[[sampler(0)]],
	sampler				samplerState1	[[sampler(1)]],
	constant Params &params [[buffer(PARAMETER_SLOT)]]
)
{
	float2 normal = 2 * (NormalMap.sample( samplerState1, inPs.uv0 * params.GlassWeight ).xy - 0.5);

	return RT.sample( samplerState0, inPs.uv0 + normal.xy * 0.05 );
}
