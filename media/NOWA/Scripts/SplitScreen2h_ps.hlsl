struct PS_INPUT
{
    float2 uv0 : TEXCOORD0;
};

Texture2D<float4> tex1 : register(t0); // Left texture
Texture2D<float4> tex2 : register(t1); // Right texture
SamplerState samplerState0 : register(s0); // Sampler for tex1
SamplerState samplerState1 : register(s1); // Sampler for tex2

float4 main(PS_INPUT inPs) : SV_Target
{
    float2 uv = inPs.uv0; // Input UV coordinates

    // Determine which texture to sample based on UV.x
    if (uv.x < 0.5) // Left half of the screen
    {
        // Scale UVs for the left texture
        uv.x *= 2.0;
        return tex1.Sample(samplerState0, uv);
    }
    else // Right half of the screen
    {
        // Scale and shift UVs for the right texture
        uv.x = (uv.x - 0.5) * 2.0;
        return tex2.Sample(samplerState1, uv);
    }
}
