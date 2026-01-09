// UnderwaterPostDepth_ps.hlsl
// Fullscreen post: underwater distortion + fog/absorption using scene depth
// Slot layout (CompositorPassQuad):
//   t0,s0 = rt0 (scene colour)
//   t1,s1 = depthTextureCopy (downscaled depth; reverse-Z friendly)

Texture2D<float4> rt0      : register(t0);
SamplerState      samp0    : register(s0);

Texture2D<float>  depthTex : register(t1);
SamplerState      samp1    : register(s1);

struct PS_INPUT
{
    float4 position : SV_Position;
    float2 uv0      : TEXCOORD0;
};

// Ogre material system passes these via params (see UnderwaterPostDepth.material)
float4 main( PS_INPUT inPs,
             uniform float timeSeconds,
             uniform float distortionAmount,
             uniform float2 distortionScale,
             uniform float2 distortionSpeed,
             uniform float3 fogColour,
             uniform float fogDensity,
             uniform float3 absorptionCoeff,
             uniform float nearClip,
             uniform float farClip ) : SV_Target0
{
    float2 uv = inPs.uv0;

    // ---------------------------------------------------------------------
    // Distortion (stable, no wrap-jitter)
    // ---------------------------------------------------------------------
    float2 w1 = sin( (uv * distortionScale) + (timeSeconds * distortionSpeed) );
    float2 w2 = sin( (uv.yx * (distortionScale * 1.37)) + (timeSeconds * (distortionSpeed * 1.73) + 1.234) );
    float2 distortion = (w1 + w2) * distortionAmount;

    float3 sceneCol = rt0.Sample( samp0, uv + distortion ).rgb;

    // ---------------------------------------------------------------------
    // Depth -> approximate view distance
    // NOTE: Ogre-Next typically uses reverse-Z (near=1, far=0) in D3D11.
    // This reconstruction assumes reverse-Z. If you disable reverse-Z,
    // swap the formula (see README).
    // ---------------------------------------------------------------------
    float d = depthTex.Sample( samp1, uv ).r;
    d = saturate( d );

    // Reverse-Z linearization (finite far):
    // viewZ = (near*far) / (near + d*(far-near))
    float denom = nearClip + d * (farClip - nearClip);
    float viewZ = (nearClip * farClip) / max( denom, 1e-6 );

    // Fog/absorption: Beer-Lambert
    // absorptionCoeff is per-channel (higher = murkier)
    float3 absorb = exp( -absorptionCoeff * viewZ );
    float3 scattered = fogColour * (1.0f - absorb);
    float3 col = sceneCol * absorb + scattered;

    // Optional extra fog push (artist knob):
    float fog = 1.0f - exp( -fogDensity * viewZ );
    col = lerp( col, fogColour, saturate(fog) );

    return float4( col, 1.0f );
}
