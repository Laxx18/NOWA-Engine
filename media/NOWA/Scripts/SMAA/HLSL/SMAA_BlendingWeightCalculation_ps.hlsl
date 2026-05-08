
#include "SMAA_HLSL.hlsl"

struct PS_INPUT
{
	float2 uv0			: TEXCOORD0;
	float2 pixcoord0    : TEXCOORD1;
	float4 offset[3]    : TEXCOORD2;
	float4 gl_Position	: SV_Position;
};

Texture2D edgeTex		: register(t0);
Texture2D areaTex		: register(t1);
Texture2D searchTex		: register(t2);

// Global declaration so D3D11 reflection can find it (uniform function params
// are not reliably exposed by ID3D11ShaderReflection in SM4/SM5).
uniform float4 viewportSize;

float4 main
(
	PS_INPUT inPs
) : SV_Target
{
	return SMAABlendingWeightCalculationPS( inPs.uv0, inPs.pixcoord0, inPs.offset,
											edgeTex, areaTex, searchTex,
											float4( 0, 0, 0, 0 ) SMAA_EXTRA_PARAM_ARG );
}
