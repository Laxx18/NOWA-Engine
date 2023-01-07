#include <metal_stdlib>
using namespace metal;

struct PS_INPUT
{
	float2 uv0;
};

struct Params
{
	float speedX;
	float speedY;
};

fragment float4 main_metal
(
	PS_INPUT inPs [[stage_in]],
	texture2d<float>	RT				[[texture(0)]],
	sampler				samplerState	[[sampler(0)]],
	constant Params &p [[buffer(PARAMETER_SLOT)]]
)
{
	inPs.uv0.x += p.speedX;
	inPs.uv0.y += p.speedY;
	return RT.sample(samplerState, inPs.uv0);
}
