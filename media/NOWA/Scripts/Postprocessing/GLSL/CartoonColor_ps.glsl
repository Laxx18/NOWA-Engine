#version ogre_glsl_ver_330

//-------------------------------
// Cartoon: Cel-shade + edge composite pass
//
// Parameters:
//   numBands      - number of discrete colour levels per channel, e.g. 4.0
//   saturation    - colour saturation multiplier (>1 = more vivid), e.g. 1.4
//   edgeDarkness  - brightness of edge pixels (0 = pure black, 1 = no darkening), e.g. 0.0
//
// RT      - original scene colour (texture unit 0)
// EdgeTex - edge mask from CartoonEdge pass (texture unit 1)
//-------------------------------

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan_layout( ogre_t1 ) uniform texture2D EdgeTex;

vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform float numBands;
    uniform float saturation;
    uniform float edgeDarkness;
vulkan( }; )

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
    vec2 uv0;
} inPs;

vec3 adjustSaturation( vec3 color, float sat )
{
    float grey = dot( color, vec3( 0.299, 0.587, 0.114 ) );
    return mix( vec3( grey ), color, sat );
}

void main()
{
    vec3  color = texture( vkSampler2D( RT,      samplerState ), inPs.uv0 ).rgb;
    float edge  = texture( vkSampler2D( EdgeTex, samplerState ), inPs.uv0 ).r;

    // Quantise each channel into discrete cel bands
    color = floor( color * numBands + 0.5 ) / numBands;

    // Boost saturation for that vibrant hand-painted feel
    color = adjustSaturation( color, saturation );

    // Darken pixels where an edge was detected
    // edge == 0 → no darkening; edge == 1 → multiply by edgeDarkness
    color *= mix( 1.0, edgeDarkness, edge );

    fragColour = vec4( color, 1.0 );
}
