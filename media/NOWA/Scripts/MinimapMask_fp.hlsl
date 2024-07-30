struct PS_INPUT
{
    float2 uv0 : TEXCOORD0;
};

Texture2D<float4> MinimapRT : register(t0);
SamplerState samplerState0 : register(s0);

float4 main(PS_INPUT inPs) : SV_Target
{
    float2 center = float2(0.5, 0.5);
    float radius = 0.5;

    float dist = distance(inPs.uv0, center);
    if (dist > radius)
    {
        discard;
    }

    float4 color = MinimapRT.Sample(samplerState0, inPs.uv0);
    return color;
}
