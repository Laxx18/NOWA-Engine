Texture2D<float3> RT		: register(t0);
SamplerState samplerState	: register(s0);

float4 main( float2 uv0 : TEXCOORD0,
			uniform float3 color )
			: SV_Target
{
	float greyscale = dot( RT.Sample(samplerState, uv0).xyz, color );
	return float4(greyscale.xxx, 1.0);
}
