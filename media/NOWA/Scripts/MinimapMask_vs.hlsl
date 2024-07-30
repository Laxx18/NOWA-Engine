struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};

VS_OUTPUT main(float4 inPos : POSITION, float2 inTex : TEXCOORD0)
{
    VS_OUTPUT output;
    output.Pos = inPos;
    output.Tex = inTex;
    return output;
}
