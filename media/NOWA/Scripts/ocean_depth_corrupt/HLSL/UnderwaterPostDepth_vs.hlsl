struct VS_INPUT
{
    uint vertexID : SV_VertexID;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT outVs;

    // Fullscreen triangle (no vertex buffer)
    float2 pos;
    pos.x = (input.vertexID == 2) ? 3.0f : -1.0f;
    pos.y = (input.vertexID == 1) ? -3.0f : 1.0f;

    outVs.position = float4(pos, 0.0f, 1.0f);

    // Map to UV space
    outVs.uv = 0.5f * (pos + 1.0f);

    return outVs;
}
