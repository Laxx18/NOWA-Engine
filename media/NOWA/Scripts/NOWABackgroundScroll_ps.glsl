#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform texture2D backgroundMap0;
vulkan_layout( ogre_t1 ) uniform texture2D backgroundMap1;
vulkan_layout( ogre_t2 ) uniform texture2D backgroundMap2;
vulkan_layout( ogre_t3 ) uniform texture2D backgroundMap3;
vulkan_layout( ogre_t4 ) uniform texture2D backgroundMap4;
vulkan_layout( ogre_t5 ) uniform texture2D backgroundMap5;
vulkan_layout( ogre_t6 ) uniform texture2D backgroundMap6;
vulkan_layout( ogre_t7 ) uniform texture2D backgroundMap7;
vulkan_layout( ogre_t8 ) uniform texture2D backgroundMap8;
vulkan_layout( ogre_s0 ) uniform sampler samplerState;

vulkan_layout( location = 0 )
in block
{
    vec2 uv0;
} inPs;

vulkan_layout( location = 0 )
out vec4 fragColour;

in vec4 gl_FragCoord;

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform float speedsX[9];
    uniform float speedsY[9];
    uniform float layerEnabled[9];
vulkan( }; )

void main()
{
    vec4 result = vec4(0.0, 0.0, 0.0, 0.0);
    
    // Create array of texture samplers
    texture2D maps[9] = texture2D[9](
        backgroundMap0, backgroundMap1, backgroundMap2,
        backgroundMap3, backgroundMap4, backgroundMap5,
        backgroundMap6, backgroundMap7, backgroundMap8
    );
    
    vec2 uv0 = inPs.uv0;
    
    for (int i = 0; i < 9; ++i)
    {
        float enabled = layerEnabled[i];
        if (enabled > 0.5)
        {
            vec2 scroll = vec2(
                speedsX[i],
                speedsY[i]
            );
            
            vec2 uv = uv0 + scroll;
            vec4 layer = texture(sampler2D(maps[i], samplerState), uv);
            result.rgb = mix(result.rgb, layer.rgb, layer.a);
            result.a = max(result.a, layer.a);
        }
    }
    
    fragColour = result;
}