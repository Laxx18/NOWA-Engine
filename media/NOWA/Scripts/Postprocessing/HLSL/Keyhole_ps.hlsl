
Texture2D Keyhole_Mask : register(t0);  // Keyhole mask texture
SamplerState samplerState : register(s0);

float4 main
(
    float2 texCoord  : TEXCOORD0
) : SV_Target
{
    // Just output the sampled texture color for debugging
	float4 image = Keyhole_Mask.Sample(samplerState, texCoord);
    return image;
}

