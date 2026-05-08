#version ogre_glsl_ver_330

//-------------------------------
// Depth of Field — Hexagonal Bokeh Combine
//
// Averages the three directional blur results (0°, 60°, −60°) into a
// single hexagonal bokeh kernel.  The average of three separable blurs
// along those axes approximates convolution with a regular hexagon,
// producing the characteristic 6-sided out-of-focus highlight shapes
// seen in anamorphic lens photography.
//
// RT0 — blur result along  0° (vertical,    dir = (0, 1))
// RT1 — blur result along 60° (dir ≈ (0.866, 0.5))
// RT2 — blur result along−60° (dir ≈ (0.866,−0.5))
//-------------------------------

vulkan_layout( ogre_t0 ) uniform texture2D RT0;
vulkan_layout( ogre_t1 ) uniform texture2D RT1;
vulkan_layout( ogre_t2 ) uniform texture2D RT2;

vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block { vec2 uv0; } inPs;

void main()
{
    vec4 s0 = texture( vkSampler2D( RT0, samplerState ), inPs.uv0 );
    vec4 s1 = texture( vkSampler2D( RT1, samplerState ), inPs.uv0 );
    vec4 s2 = texture( vkSampler2D( RT2, samplerState ), inPs.uv0 );

    // Simple average — CoC in alpha is averaged too for the blend pass
    fragColour = (s0 + s1 + s2) * (1.0 / 3.0);
}
