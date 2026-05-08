
#include "SMAA_HLSL.hlsl"

struct PS_INPUT
{
	float2 uv0			: TEXCOORD0;
	float4 offset       : TEXCOORD1;
};

Texture2D<float4> rt_input			: register(t0); //Can be sRGB
Texture2D<float4> blendTex			: register(t1);
#if SMAA_REPROJECTION
	Texture2D<float4> velocityTex	: register(t2);
#endif

// Global declaration so D3D11 reflection can find it (uniform function params
// are not reliably exposed by ID3D11ShaderReflection in SM4/SM5).
uniform float4 viewportSize;

float4 main
(
	PS_INPUT inPs
) : SV_Target
{
#if SMAA_REPROJECTION
	return SMAANeighborhoodBlendingPS( inPs.uv0, inPs.offset,
									   rt_input, blendTex, velocityTex );
#else
	return SMAANeighborhoodBlendingPS( inPs.uv0, inPs.offset,
									   rt_input, blendTex SMAA_EXTRA_PARAM_ARG );
#endif
}
