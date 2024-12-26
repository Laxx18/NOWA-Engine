struct PS_INPUT
{
    float2 uv0 : TEXCOORD0;
};

Texture2D<float4> tex1 : register(t0);
SamplerState samplerState1 : register(s0);
Texture2D<float4> tex2 : register(t1);
SamplerState samplerState2 : register(s1);
Texture2D<float4> tex3 : register(t2);
SamplerState samplerState3 : register(s2);
Texture2D<float4> tex4 : register(t3);
SamplerState samplerState4 : register(s3);
float4 main(PS_INPUT inPs, uniform float4 geomData1, uniform float4 geomData2, uniform float4 geomData3, uniform float4 geomData4) : SV_Target
{
    float2 uv = inPs.uv0;
	
	if (any(geomData1 != float4(0.0, 0.0, 0.0, 0.0))) {
		if (uv.x >= geomData1.x && uv.x < (geomData1.x + geomData1.z) &&
			uv.y >= geomData1.y && uv.y < (geomData1.y + geomData1.w)) {
			float2 adjustedUV = (uv - float2(geomData1.x, geomData1.y)) / float2(geomData1.z, geomData1.w);
			return tex1.Sample(samplerState1, adjustedUV);
		}
	}
	
	if (any(geomData2 != float4(0.0, 0.0, 0.0, 0.0))) {
		if (uv.x >= geomData2.x && uv.x < (geomData2.x + geomData2.z) &&
			uv.y >= geomData2.y && uv.y < (geomData2.y + geomData2.w)) {
			float2 adjustedUV = (uv - float2(geomData2.x, geomData2.y)) / float2(geomData2.z, geomData2.w);
			return tex2.Sample(samplerState2, adjustedUV);
		}
	}
	
	if (any(geomData3 != float4(0.0, 0.0, 0.0, 0.0))) {
		if (uv.x >= geomData3.x && uv.x < (geomData3.x + geomData3.z) &&
			uv.y >= geomData3.y && uv.y < (geomData3.y + geomData3.w)) {
			float2 adjustedUV = (uv - float2(geomData3.x, geomData3.y)) / float2(geomData3.z, geomData3.w);
			return tex3.Sample(samplerState3, adjustedUV);
		}
	}
	
	if (any(geomData4 != float4(0.0, 0.0, 0.0, 0.0))) {
		if (uv.x >= geomData4.x && uv.x < (geomData4.x + geomData4.z) &&
			uv.y >= geomData4.y && uv.y < (geomData4.y + geomData4.w)) {
			float2 adjustedUV = (uv - float2(geomData4.x, geomData4.y)) / float2(geomData4.z, geomData4.w);
			return tex4.Sample(samplerState4, adjustedUV);
		}
	}
	
    return float4(0, 0, 0, 1);
}
