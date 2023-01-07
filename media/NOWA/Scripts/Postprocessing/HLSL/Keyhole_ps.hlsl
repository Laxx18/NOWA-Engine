Texture2D Image	: register(t0);

SamplerState samplerState		: register(s0);

float4 main
(
	float2 uv0 						: TEXCOORD0,
	uniform float frameShape		: register(c0),
	uniform float sin_time_0_X		: register(c1)

) : SV_Target
{
   // Define a frame shape
   float2 pos = abs((uv0 - 0.5) * 2.0);
   float f = (1 - pos.x * pos.x) * (1 - pos.y * pos.y);
   float frame = saturate(pow(abs(f), frameShape));

   // Repeat a 1 - x^2 (0 < x < 1) curve and roll it with sinus.
   float dst = frac(pos.y + sin_time_0_X);
   dst *= (1 - dst);
   // Make sure distortion is highest in the center of the image
   dst /= 1 + abs(pos.y);

   uv0.x += dst;
   float4 image = Image.Sample(samplerState, uv0);

   return frame + image;
}
