
Texture2D Keyhole_Mask : register(t0);  // Keyhole mask texture
SamplerState samplerState : register(s0);

float4 main
(
	float2 texCoord  : TEXCOORD0,
	uniform float2 screenSize,    	// Screen dimensions
	uniform float2 center,  		// Keyhole center
	uniform float  radius,  		// Keyhole radius (normalized 0.0 - 1.0)
	uniform int    direction     	// 1 for growing, 0 for shrinking
) : SV_Target
{
    // Calculate position relative to the keyhole center
    float2 position = (texCoord * screenSize) - center;

    // Convert the radius to screen space
    float normalizedRadius = radius * length(screenSize);

    // Distance from the center
    float dist = length(position);

    // Sample the mask texture
    float4 maskValue = Keyhole_Mask.Sample(samplerState, texCoord).rgba;

    // Check if the current pixel is inside or outside the keyhole
    if (dist > normalizedRadius)
    {
        return float4(0.0, 0.0, 0.0, 1.0);  // Outside the keyhole (black)
    }
    else
    {
        return float4(1.0, 1.0, 1.0, 1.0) * maskValue;  // Inside the keyhole modulated by the mask
    }
}

