Texture2D backgroundMap3		: register(t0);
SamplerState samplerState	: register(s0);
float speedX;
float speedY;

float4 main(float2 uv0 : TEXCOORD0) : SV_Target
{
	// Apply scroll
	uv0.x += speedX;
	uv0.y += speedY;
	return backgroundMap3.Sample( samplerState, uv0 );
}
