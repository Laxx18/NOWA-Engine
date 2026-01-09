#version 330 core
// UnderwaterPostDepth_ps.glsl (combined samplers version)
// If your Ogre GLSL setup uses separate texture/sampler objects,
// you may need to adjust bindings to match your framework.

uniform sampler2D rt0;
uniform sampler2D depthTex;

uniform float timeSeconds;
uniform float distortionAmount;
uniform vec2  distortionScale;
uniform vec2  distortionSpeed;
uniform vec3  fogColour;
uniform float fogDensity;
uniform vec3  absorptionCoeff;
uniform float nearClip;
uniform float farClip;

in vec2 uv0;
out vec4 fragColor;

void main()
{
    vec2 uv = uv0;

    vec2 w1 = sin( (uv * distortionScale) + (timeSeconds * distortionSpeed) );
    vec2 w2 = sin( (uv.yx * (distortionScale * 1.37)) + (timeSeconds * (distortionSpeed * 1.73) + 1.234) );
    vec2 distortion = (w1 + w2) * distortionAmount;

    vec3 sceneCol = texture( rt0, uv + distortion ).rgb;

    float d = clamp( texture( depthTex, uv ).r, 0.0, 1.0 );

    // Reverse-Z linearization (finite far)
    float denom = nearClip + d * (farClip - nearClip);
    float viewZ = (nearClip * farClip) / max( denom, 1e-6 );

    vec3 absorb = exp( -absorptionCoeff * viewZ );
    vec3 scattered = fogColour * (vec3(1.0) - absorb);
    vec3 col = sceneCol * absorb + scattered;

    float fog = 1.0 - exp( -fogDensity * viewZ );
    col = mix( col, fogColour, clamp(fog, 0.0, 1.0) );

    fragColor = vec4( col, 1.0 );
}
