#version ogre_glsl_ver_330

//-------------------------------
// Depth of Field — CoC Pass
//
// Linearises depth and computes a per-pixel Circle of Confusion (CoC)
// that drives all subsequent blur passes.
//
// CoC model (intuitive game-dev parameters):
//   Near blur: pixels in front of focusDistance transition over nearBlurRange.
//   Far  blur: pixels behind focusDistance transition over farBlurRange.
//   In-focus band around focusDistance = no blur.
//
//   CoC is stored in [0..1] in the alpha channel of the half-res output.
//   RGB stores the scene colour downsampled to half-res.
//
// Parameters:
//   projectionParams  – (A, B) from camera->getProjectionParamsAB(), B/=far
//   farClipDistance   – camera far clip (world units)
//   focusDistance     – distance to the in-focus plane (world units, from C++)
//   nearBlurRange     – distance range over which near blur ramps up
//   farBlurRange      – distance range over which far blur ramps up
//-------------------------------

vulkan_layout( ogre_t0 ) uniform texture2D RT;
vulkan_layout( ogre_t1 ) uniform texture2D DepthTex;

vulkan( layout( ogre_s0 ) uniform sampler samplerBilinear );
vulkan( layout( ogre_s1 ) uniform sampler samplerPoint   );

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform vec2  projectionParams;
    uniform float farClipDistance;
    uniform float focusDistance;
    uniform float nearBlurRange;
    uniform float farBlurRange;
vulkan( }; )

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block { vec2 uv0; } inPs;

float lineariseDepth( float raw )
{
    float d = raw - projectionParams.x;
    if ( abs(d) < 0.0001 ) d = 0.0001;
    return (projectionParams.y / d) * farClipDistance;
}

void main()
{
    vec3  color    = texture( vkSampler2D( RT,       samplerBilinear ), inPs.uv0 ).rgb;
    float rawDepth = texture( vkSampler2D( DepthTex, samplerPoint    ), inPs.uv0 ).r;

    // Sky pixels at max depth: no blur
    float isSky    = step( 0.9999, rawDepth );

    float linDepth = lineariseDepth( rawDepth );

    float cocFar  = clamp( (linDepth  - focusDistance) / max(farBlurRange,  0.001), 0.0, 1.0 );
    float cocNear = clamp( (focusDistance - linDepth)  / max(nearBlurRange, 0.001), 0.0, 1.0 );

    float coc = max( cocFar, cocNear ) * (1.0 - isSky);

    fragColour = vec4( color, coc );
}
