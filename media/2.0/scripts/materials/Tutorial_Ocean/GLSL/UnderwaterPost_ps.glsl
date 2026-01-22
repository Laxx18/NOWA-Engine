#version ogre_glsl_ver_330

vulkan_layout( ogre_t0 ) uniform sampler2D rt0;

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform float underwaterFactor;
    uniform float timeSeconds;
    uniform vec3  waterTint;
    uniform float distortion;
    uniform float contrast;
    uniform float saturation;
    uniform float vignette;
vulkan( }; )

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
    vec2 uv0;
} inPs;

vec3 applySaturation( vec3 c, float s )
{
    float l = dot( c, vec3( 0.2126, 0.7152, 0.0722 ) );
    return mix( vec3( l ), c, s );
}

void main()
{
    vec2 uv = inPs.uv0;

    // Cheap moving distortion (no extra textures)
    float n1 = sin( (uv.x + timeSeconds * 0.25) * 40.0 );
    float n2 = sin( (uv.y + timeSeconds * 0.17) * 35.0 );
    float n  = n1 * n2;

    vec2 duv = vec2( n, -n ) * (distortion * underwaterFactor);

    vec3 col = texture( rt0, uv + duv ).rgb;

    // Absorption tint
    col = mix( col, waterTint, 0.35 * underwaterFactor );

    // Contrast around mid grey
    float contr = mix( 1.0, contrast, underwaterFactor );
    col = (col - 0.5) * contr + 0.5;

    // Saturation
    float sat = mix( 1.0, saturation, underwaterFactor );
    col = applySaturation( col, sat );

    // Vignette
    vec2 d = abs( uv - 0.5 ) * 2.0;
    float v = clamp( 1.0 - dot( d, d ) * vignette, 0.0, 1.0 );
    col *= mix( 1.0, v, underwaterFactor );

    fragColour = vec4( col, 1.0 );
}
