#include <metal_stdlib>
using namespace metal;

struct PS_INPUT
{
	float2 uv0;
};

struct Params
{
	float4	color;
};

fragment float4 main_metal
(
	PS_INPUT inPs [[stage_in]],
	texture2d<float>	RT				[[texture(0)]],
	sampler				samplerState	[[sampler(0)]],
	constant Params &params [[buffer(PARAMETER_SLOT)]]
)
{
	float greyscale = dot( RT.sample(samplerState, inPs.uv0).xyz, params.color );
	return float4( greyscale, greyscale, greyscale, 1.0 );
}
