#include <metal_stdlib>
using namespace metal;

#define gl_FragCoord inPs.inputPos

struct PS_INPUT
{
    float2 uv0;
    float4 inputPos [[position]];
};

struct Params
{
    float speedsX[9];
    float speedsY[9];
    float layerEnabled[9];
};

fragment float4 main_metal
(
    PS_INPUT inPs [[stage_in]],
    constant Params &p [[buffer(PARAMETER_SLOT)]],
    texture2d<float> backgroundMap0 [[texture(0)]],
    texture2d<float> backgroundMap1 [[texture(1)]],
    texture2d<float> backgroundMap2 [[texture(2)]],
    texture2d<float> backgroundMap3 [[texture(3)]],
    texture2d<float> backgroundMap4 [[texture(4)]],
    texture2d<float> backgroundMap5 [[texture(5)]],
    texture2d<float> backgroundMap6 [[texture(6)]],
    texture2d<float> backgroundMap7 [[texture(7)]],
    texture2d<float> backgroundMap8 [[texture(8)]],
    sampler samplerState [[sampler(0)]]
)
{
    float4 result = float4(0.0, 0.0, 0.0, 0.0);
    
    // Create array of textures
    array<texture2d<float>, 9> maps = {
        backgroundMap0, backgroundMap1, backgroundMap2,
        backgroundMap3, backgroundMap4, backgroundMap5,
        backgroundMap6, backgroundMap7, backgroundMap8
    };
    
    float2 uv0 = inPs.uv0;
    
    for (int i = 0; i < 9; ++i)
    {
        float enabled = p.layerEnabled[i];
        if (enabled > 0.5f)
        {
            float2 scroll = float2(
                p.speedsX[i],
                p.speedsY[i]
            );
            
            float2 uv = uv0 + scroll;
            float4 layer = maps[i].sample(samplerState, uv);
            result.rgb = mix(result.rgb, layer.rgb, layer.a);
            result.a = max(result.a, layer.a);
        }
    }
    
    return result;
}