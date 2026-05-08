#version ogre_glsl_ver_330

//-------------------------------
// Light Shafts — Pass 1: Occlusion Mask
//
// Builds a half-resolution emission mask that drives the radial blur:
//
//   Sky pixels (rawDepth ≈ 1.0) near the sun that are also bright in the
//   colour buffer → WHITE emitters.
//
//   Any pixel where geometry occludes the sky (rawDepth < occlusionDepthThreshold)
//   → BLACK occluder.  This is what makes shafts get cut by mountains / trees.
//
//   The double gate (depth AND colour) means the sun must actually be visible
//   as a bright area in the sky for shafts to appear — night scenes produce
//   nothing automatically.
//
// Parameters:
//   sunScreenPos           - sun UV position, updated per-frame from C++
//   occlusionDepthThreshold - rawDepth below this = geometry = occluder, e.g. 0.9999
//   sunRadius              - screen-space radius around sun for emission mask
//   brightnessThreshold    - minimum luminance to count as a sky emitter, e.g. 0.6
//-------------------------------

vulkan_layout( ogre_t0 ) uniform texture2D RT;        // scene colour
vulkan_layout( ogre_t1 ) uniform texture2D DepthTex;  // PFG_D32_FLOAT depth

vulkan( layout( ogre_s0 ) uniform sampler samplerBilinear );
vulkan( layout( ogre_s1 ) uniform sampler samplerPoint   );

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform vec2  sunScreenPos;
    uniform float occlusionDepthThreshold;
    uniform float sunRadius;
    uniform float brightnessThreshold;
vulkan( }; )

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
    vec2 uv0;
} inPs;

void main()
{
    float rawDepth = texture( vkSampler2D( DepthTex, samplerPoint    ), inPs.uv0 ).r;
    vec3  color    = texture( vkSampler2D( RT,       samplerBilinear ), inPs.uv0 ).rgb;

    // Gate 1: geometry occlusion — any pixel with geometry in front is a hard blocker
    float isSky = step( rawDepth, occlusionDepthThreshold );

    // Gate 2: brightness — only actually bright sky pixels emit
    float lum        = dot( color, vec3( 0.2126, 0.7152, 0.0722 ) );
    float isBright   = step( brightnessThreshold, lum );

    // Gate 3: proximity to sun — soft radial falloff
    float sunDist    = length( inPs.uv0 - sunScreenPos );
    float radialMask = 1.0 - smoothstep( 0.0, sunRadius, sunDist );

    // Emission = sky AND bright, intensity modulated by radial falloff
    float emission = isSky * isBright * (0.3 + radialMask * 0.7);

    fragColour = vec4( color * emission, 1.0 );
}
