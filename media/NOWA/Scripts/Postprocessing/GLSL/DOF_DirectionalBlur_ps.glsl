#version ogre_glsl_ver_330

//-------------------------------
// Depth of Field — Directional Blur
//
// Shared by both Gaussian DOF and Hexagonal Bokeh DOF.
//
// Gaussian:  called twice with blurDir=(1,0) then blurDir=(0,1).
// Hex Bokeh: called three times with blurDir at 0°, 60°, −60°.
//
// The kernel size per-pixel is CoC * blurRadius * texelSize.
// Pixels with CoC ≈ 0 are sampled with a 1-tap (no blur), reducing
// cost and keeping in-focus areas perfectly sharp.
//
// RT is the RGBA output of the CoC pass: RGB = colour, A = CoC [0..1].
//
// Parameters:
//   blurDir    – normalised direction vector in UV space
//   blurRadius – maximum kernel radius in UV units (e.g. 0.01)
//   numSamples – tap count per side (total = 2*numSamples+1), e.g. 8
//-------------------------------

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan( layout( ogre_s0 ) uniform sampler samplerState );

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform vec2  blurDir;
    uniform float blurRadius;
    uniform float numSamples;   // float for shader arithmetic convenience
vulkan( }; )

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block { vec2 uv0; } inPs;

void main()
{
    vec4  center  = texture( vkSampler2D( RT, samplerState ), inPs.uv0 );
    float coc     = center.a;

    // No-op for in-focus pixels
    if ( coc < 0.005 )
    {
        fragColour = center;
        return;
    }

    float stepSize  = (blurRadius * coc) / numSamples;
    vec2  stepVec   = blurDir * stepSize;

    vec4  accumulated = vec4( 0.0 );
    float totalWeight = 0.0;

    int iSamples = int(numSamples);

    for ( int i = -iSamples; i <= iSamples; i++ )
    {
        vec2  sampleUV    = inPs.uv0 + stepVec * float(i);
        vec4  s           = texture( vkSampler2D( RT, samplerState ), sampleUV );

        // Weight by sample CoC so near-field blur bleeds over sharp objects
        float w = max( s.a, coc * 0.5 );
        accumulated  += s * w;
        totalWeight  += w;
    }

    fragColour = accumulated / max( totalWeight, 0.0001 );
}
