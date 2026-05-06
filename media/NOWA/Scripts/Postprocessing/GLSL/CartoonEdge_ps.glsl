#version ogre_glsl_ver_330

//-------------------------------
// Cartoon: Sobel edge detection pass
// Outputs a grayscale edge mask (bright = edge, dark = flat).
// texelSize is fed via param_named_auto (inverse_texture_size).
//-------------------------------

vulkan_layout( ogre_t0 ) uniform texture2D RT;

vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform vec4  texelSize;     // (1/w, 1/h, w, h) from inverse_texture_size auto-param
    uniform float edgeThreshold; // minimum gradient magnitude to count as an edge, e.g. 0.1
    uniform float edgeStrength;  // final edge intensity multiplier, e.g. 1.0
vulkan( }; )

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
    vec2 uv0;
} inPs;

float luminance( vec3 c )
{
    return dot( c, vec3( 0.299, 0.587, 0.114 ) );
}

void main()
{
    vec2 uv = inPs.uv0;
    vec2 ts = texelSize.xy;

    // Sample 3x3 neighbourhood luminances
    float tl = luminance( texture( vkSampler2D( RT, samplerState ), uv + vec2( -ts.x,  ts.y ) ).rgb );
    float tm = luminance( texture( vkSampler2D( RT, samplerState ), uv + vec2(  0.0,   ts.y ) ).rgb );
    float tr = luminance( texture( vkSampler2D( RT, samplerState ), uv + vec2(  ts.x,  ts.y ) ).rgb );
    float ml = luminance( texture( vkSampler2D( RT, samplerState ), uv + vec2( -ts.x,  0.0  ) ).rgb );
    float mr = luminance( texture( vkSampler2D( RT, samplerState ), uv + vec2(  ts.x,  0.0  ) ).rgb );
    float bl = luminance( texture( vkSampler2D( RT, samplerState ), uv + vec2( -ts.x, -ts.y ) ).rgb );
    float bm = luminance( texture( vkSampler2D( RT, samplerState ), uv + vec2(  0.0,  -ts.y ) ).rgb );
    float br = luminance( texture( vkSampler2D( RT, samplerState ), uv + vec2(  ts.x, -ts.y ) ).rgb );

    // Sobel kernels
    float gx = -tl - 2.0 * ml - bl + tr + 2.0 * mr + br;
    float gy = -tl - 2.0 * tm - tr + bl + 2.0 * bm + br;

    float edge = sqrt( gx * gx + gy * gy );
    edge = step( edgeThreshold, edge ) * edgeStrength;

    fragColour = vec4( edge, edge, edge, 1.0 );
}
