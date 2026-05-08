#version ogre_glsl_ver_330

//-------------------------------
// Outline / Silhouette Effect
//
// Runs a Sobel operator on the LINEARISED depth buffer to detect
// geometry silhouettes only.  Because depth (not colour) drives the
// edge test, texture detail is completely ignored — only actual
// geometry discontinuities produce lines.
//
// This is the correct fix for the colour-Sobel cartoon problem where
// textured terrain produced noise everywhere.
//
// Parameters:
//   projectionParams  – (A, B/far) depth linearisation, pushed from C++
//   farClipDistance   – camera far clip
//   texelSize         – (1/w, 1/h, w, h) from inverse_texture_size auto-param
//   outlineColor      – RGB colour of outline strokes (default: black)
//   outlineThickness  – Sobel neighbourhood step multiplier (1 = 1 px, 2 = 2 px, …)
//   depthThreshold    – minimum normalised depth gradient to register as an edge
//   outlineStrength   – final opacity multiplier for the outline layer [0..1]
//-------------------------------

vulkan_layout( ogre_t0 ) uniform texture2D RT;        // scene colour
vulkan_layout( ogre_t1 ) uniform texture2D DepthTex;  // PFG_D32_FLOAT depth

vulkan( layout( ogre_s0 ) uniform sampler samplerBilinear );
vulkan( layout( ogre_s1 ) uniform sampler samplerPoint   );

vulkan( layout( ogre_P0 ) uniform Params { )
    uniform vec2  projectionParams;
    uniform float farClipDistance;
    uniform vec4  texelSize;       // (1/w, 1/h, w, h) from inverse_texture_size
    uniform vec3  outlineColor;
    uniform float outlineThickness;
    uniform float depthThreshold;
    uniform float outlineStrength;
vulkan( }; )

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block { vec2 uv0; } inPs;

// Linearise raw depth to [0..1] range (0=near, 1=far)
float lineariseDepth( float raw )
{
    float d = raw - projectionParams.x;
    if ( abs(d) < 0.0001 ) d = 0.0001;
    // Divide by farClipDistance to normalise to [0..1]
    return clamp( (projectionParams.y / d), 0.0, 1.0 );
}

void main()
{
    vec3  sceneColor = texture( vkSampler2D( RT, samplerBilinear ), inPs.uv0 ).rgb;

    vec2 ts = texelSize.xy * outlineThickness;

    // Sample 3x3 depth neighbourhood
    float tl = lineariseDepth( texture( vkSampler2D( DepthTex, samplerPoint ), inPs.uv0 + vec2(-ts.x,  ts.y) ).r );
    float tm = lineariseDepth( texture( vkSampler2D( DepthTex, samplerPoint ), inPs.uv0 + vec2( 0.0,   ts.y) ).r );
    float tr = lineariseDepth( texture( vkSampler2D( DepthTex, samplerPoint ), inPs.uv0 + vec2( ts.x,  ts.y) ).r );
    float ml = lineariseDepth( texture( vkSampler2D( DepthTex, samplerPoint ), inPs.uv0 + vec2(-ts.x,  0.0 ) ).r );
    float mr = lineariseDepth( texture( vkSampler2D( DepthTex, samplerPoint ), inPs.uv0 + vec2( ts.x,  0.0 ) ).r );
    float bl = lineariseDepth( texture( vkSampler2D( DepthTex, samplerPoint ), inPs.uv0 + vec2(-ts.x, -ts.y) ).r );
    float bm = lineariseDepth( texture( vkSampler2D( DepthTex, samplerPoint ), inPs.uv0 + vec2( 0.0,  -ts.y) ).r );
    float br = lineariseDepth( texture( vkSampler2D( DepthTex, samplerPoint ), inPs.uv0 + vec2( ts.x, -ts.y) ).r );

    // Sobel X and Y kernels on linearised depth
    float gx = -tl - 2.0 * ml - bl + tr + 2.0 * mr + br;
    float gy = -tl - 2.0 * tm - tr + bl + 2.0 * bm + br;

    float gradient = sqrt( gx * gx + gy * gy );

    // Hard threshold → clean silhouette lines without haloing
    float edge = step( depthThreshold, gradient ) * outlineStrength;

    // Composite: blend outline colour over scene
    vec3 result = mix( sceneColor, outlineColor, edge );

    fragColour = vec4( result, 1.0 );
}
