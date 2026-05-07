#version ogre_glsl_ver_330

//-------------------------------
// Depth Fog + Height Fog compositor effect
//
// Depth fog:   exponential fog based on linearised camera distance.
//              Density controlled by depthFogDensity.
//
// Height fog:  exponential fog based on world-space Y, allowing ground-hugging
//              mist or elevated cloud layers. Density controlled by heightFogDensity.
//
// Both fog types are blended additively and then mixed with the scene colour.
// Setting either density to 0 effectively disables that layer.
//
// Depth reconstruction uses the same projectionParams / farClipDistance
// formula as SSAO and Underwater — values set from C++ in postInit / update.
//
// World-space Y is reconstructed via the camera's inverse view-projection
// matrix, also pushed per-frame from C++ in update().
//-------------------------------

vulkan_layout( ogre_t0 ) uniform texture2D RT;           // scene colour
vulkan_layout( ogre_t1 ) uniform texture2D DepthTex;     // PFG_D32_FLOAT depth

vulkan( layout( ogre_s0 ) uniform sampler samplerBilinear );
vulkan( layout( ogre_s1 ) uniform sampler samplerPoint   );

vulkan( layout( ogre_P0 ) uniform Params { )
    // Depth linearisation (same as SSAO / Underwater)
    uniform vec2  projectionParams;   // (A, B) from camera->getProjectionParamsAB()
    uniform float farClipDistance;

    // World-space reconstruction
    uniform mat4  invViewProjMatrix;  // updated per frame from C++

    // Depth fog
    uniform float depthFogDensity;    // e.g. 0.01  (set 0 to disable)
    uniform float depthFogStart;      // linear depth before fog begins, e.g. 10.0

    // Height fog
    uniform float heightFogDensity;   // e.g. 0.08  (set 0 to disable)
    uniform float heightFogStart;     // world Y below this is fully fogged, e.g. 0.0
    uniform float heightFogEnd;       // world Y above this has no height fog,  e.g. 30.0

    // Shared
    uniform vec3  fogColor;           // e.g. (0.7, 0.75, 0.8) cool grey-blue
    uniform float fogSkyBlend;        // 0 = sharp horizon, 1 = soft sky bleed [0..1]
vulkan( }; )

vulkan_layout( location = 0 )
out vec4 fragColour;

vulkan_layout( location = 0 )
in block
{
    vec2 uv0;
} inPs;

// ---------------------------------------------------------------------------
// Linearise depth — identical to Underwater / SSAO implementation
// ---------------------------------------------------------------------------
float getLinearDepth(float rawDepth)
{
    float denom = rawDepth - projectionParams.x;
    if (abs(denom) < 0.0001)
        denom = 0.0001;
    return (projectionParams.y / denom) * farClipDistance;
}

// ---------------------------------------------------------------------------
// Reconstruct world-space position from UV + raw depth
// ---------------------------------------------------------------------------
vec3 getWorldPos(vec2 uv, float rawDepth)
{
    // Flip Y: Ogre UV (0,0) = top-left, NDC (−1,−1) = bottom-left
    vec4 ndcPos = vec4(uv.x * 2.0 - 1.0,
                       (1.0 - uv.y) * 2.0 - 1.0,
                       rawDepth * 2.0 - 1.0,   // GL NDC Z in [-1,1]
                       1.0);
    vec4 worldPos = invViewProjMatrix * ndcPos;
    return worldPos.xyz / worldPos.w;
}

void main()
{
    vec3  sceneColor = texture( vkSampler2D( RT,       samplerBilinear ), inPs.uv0 ).rgb;
    float rawDepth   = texture( vkSampler2D( DepthTex, samplerPoint    ), inPs.uv0 ).r;

    float linearDepth = getLinearDepth(rawDepth);

    // -----------------------------------------------------------------------
    // Depth fog factor — exponential, starts at depthFogStart
    // -----------------------------------------------------------------------
    float depthFogFactor = 0.0;
    if (depthFogDensity > 0.0)
    {
        float effectiveDepth = max(0.0, linearDepth - depthFogStart);
        depthFogFactor = 1.0 - exp(-depthFogDensity * effectiveDepth);
        depthFogFactor = clamp(depthFogFactor, 0.0, 1.0);
    }

    // -----------------------------------------------------------------------
    // Height fog factor — exponential falloff based on world-space Y
    // -----------------------------------------------------------------------
    float heightFogFactor = 0.0;
    if (heightFogDensity > 0.0)
    {
        vec3  worldPos = getWorldPos(inPs.uv0, rawDepth);
        float normalizedHeight = clamp(
            (worldPos.y - heightFogStart) / max(heightFogEnd - heightFogStart, 0.001),
            0.0, 1.0);
        // Invert: low altitude = more fog
        float heightFade = 1.0 - normalizedHeight;
        heightFogFactor  = 1.0 - exp(-heightFogDensity * heightFade * linearDepth * 0.1);
        heightFogFactor  = clamp(heightFogFactor, 0.0, 1.0);
    }

    // -----------------------------------------------------------------------
    // Combine both fog layers — take maximum so they don't cancel each other
    // -----------------------------------------------------------------------
    float totalFog = clamp(max(depthFogFactor, heightFogFactor), 0.0, 1.0);

    // Soft sky suppression: pixels that are at maximum depth (sky) get
    // a gentle blend reduction so fog doesn't white-out the skybox entirely
    float skySuppress = (rawDepth >= 0.9999) ? (1.0 - fogSkyBlend) : 1.0;
    totalFog *= skySuppress;

    fragColour = vec4(mix(sceneColor, fogColor, totalFog), 1.0);
}
