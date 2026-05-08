#version ogre_glsl_ver_330

//-------------------------------
// Depth of Field — Blend Pass
//
// Shared final pass for both Gaussian and Hexagonal Bokeh DOF.
//
// Sharp  = RT_Input  (original full-res scene colour)
// Blurred= BlurredRT (half-res, upsampled bilinearly on sample)
// Mask   = CoCRT     (half-res CoC pass output, alpha channel)
//
// Near-field (CoC ≈ near) blends more aggressively to let the
// blurred layer "bleed" over nearby in-focus objects — this is
// the correct near-DOF look.
//-------------------------------

vulkan_layout( ogre_t0 ) uniform texture2D RT_Input;   // full-res scene colour
vulkan_layout( ogre_t1 ) uniform texture2D BlurredRT;  // half-res blurred result
vulkan_layout( ogre_t2 ) uniform texture2D CoCRT;      // half-res CoC (alpha = CoC)

vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform float blendStrength;  // global multiplier [0..1], default 1.0
vulkan( }; )

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block { vec2 uv0; } inPs;

void main()
{
    vec3  sharp   = texture( vkSampler2D( RT_Input,  samplerState ), inPs.uv0 ).rgb;
    vec3  blurred = texture( vkSampler2D( BlurredRT, samplerState ), inPs.uv0 ).rgb;
    float coc     = texture( vkSampler2D( CoCRT,     samplerState ), inPs.uv0 ).a;

    float blend = coc * blendStrength;

    fragColour = vec4( mix( sharp, blurred, clamp(blend, 0.0, 1.0) ), 1.0 );
}
