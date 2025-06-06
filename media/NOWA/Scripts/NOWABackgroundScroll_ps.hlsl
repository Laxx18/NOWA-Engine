Texture2D backgroundMap0 : register(t0);
Texture2D backgroundMap1 : register(t1);
Texture2D backgroundMap2 : register(t2);
Texture2D backgroundMap3 : register(t3);
Texture2D backgroundMap4 : register(t4);
Texture2D backgroundMap5 : register(t5);
Texture2D backgroundMap6 : register(t6);
Texture2D backgroundMap7 : register(t7);
Texture2D backgroundMap8 : register(t8);

SamplerState samplerState : register(s0);

struct PS_INPUT
{
    float2 uv0 : TEXCOORD0;
};

// Helper function to extract float from float4 array packed as 3 x float4 (for 9 elements)
float getArrayValue(float4 array[3], int idx)
{
    if (idx < 0 || idx >= 9) return 0.0f;
    return array[idx / 4][idx % 4];
}

float4 sampleBackgroundLayer(int index, float2 uv)
{
    // Select correct texture
    switch (index)
    {
        case 0: return backgroundMap0.Sample(samplerState, uv);
        case 1: return backgroundMap1.Sample(samplerState, uv);
        case 2: return backgroundMap2.Sample(samplerState, uv);
        case 3: return backgroundMap3.Sample(samplerState, uv);
        case 4: return backgroundMap4.Sample(samplerState, uv);
        case 5: return backgroundMap5.Sample(samplerState, uv);
        case 6: return backgroundMap6.Sample(samplerState, uv);
        case 7: return backgroundMap7.Sample(samplerState, uv);
        case 8: return backgroundMap8.Sample(samplerState, uv);
        default: return float4(0, 0, 0, 0);
    }
}

float4 main(
    PS_INPUT inPs,
    uniform float4 speedsX[3],
    uniform float4 speedsY[3],
    uniform float4 layerEnabled[3],
    float4 gl_FragCoord : SV_Position
) : SV_Target
{
    float2 uv0 = inPs.uv0;
    float4 result = float4(0, 0, 0, 0);

    for (int i = 0; i < 9; ++i)
	{
		float enabled = getArrayValue(layerEnabled, i);
		if (enabled > 0.5f)
		{
			float2 scroll = float2(
				getArrayValue(speedsX, i),
				getArrayValue(speedsY, i)
			);

			float2 uv = uv0 + scroll;
			float4 layer = sampleBackgroundLayer(i, uv);

			// Premultiply alpha (optional but recommended)
			layer.rgb *= layer.a;

			// Alpha blend
			result.rgb = lerp(result.rgb, layer.rgb, layer.a);
			result.a = max(result.a, layer.a);
		}
	}

    return result;
}
