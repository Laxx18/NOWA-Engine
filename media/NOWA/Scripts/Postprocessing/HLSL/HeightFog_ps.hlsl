struct PS_INPUT
{
	float2 uv0 : TEXCOORD0;
	float4 gl_Position : SV_Position;
};

Texture2D rt_depth : register(t0);
Texture2D rt_scene : register(t1);

SamplerState SS_depth : register(s0);
SamplerState SS_scene : register(s1);

float expdiv(float x){	if (abs(x) < 0.0001){ return 1;} else { return (exp(x) - 1) / x;}}

float ExpGroundFog(float dist, float h1, float h2, float density, float verticalDecay, float baseLevel){
	float deltaH = (h2 - h1);
	float res =  (1 - exp(-density * dist * exp(verticalDecay * (baseLevel - h1)) * expdiv(-verticalDecay * deltaH)));
	return res;
}

float3 ApplyFog(float3 originalColor, float eyePosY, float3 eyeToPixel)
{
	float3 FogColor = float3(0.1, 0.4, 0.1);
	// float FogStartDepth;
	float3 FogHighlightColor = float3(0.5, 0.9, 0.2);
	float FogGlobalDensity = 0.2;
	float3 FogSunDir = float3(0.1, 0.6, 0.1);
	float FogHeightFalloff = 0.05;
	float FogStartDist = 1.0;

	float pixelDist = length(eyeToPixel);
	float3 eyeToPixelNorm = eyeToPixel / pixelDist;

	// Find the fog staring distance to pixel distance
	float fogDist = max(pixelDist - FogStartDist, 0.0);

	// Distance based fog intensity
	float fogHeightDensityAtViewer = exp(-FogHeightFalloff * eyePosY);
	float fogDistInt = fogDist * fogHeightDensityAtViewer;

	// Height based fog intensity
	float eyeToPixelY = eyeToPixel.y * (fogDist / pixelDist);
	float t = FogHeightFalloff * eyeToPixelY;
	const float thresholdT = 0.01;
	float fogHeightInt = abs(t) > thresholdT ? (1.0 - exp(-t)) / t : 1.0;

	// Combine both factors to get the final factor
	float fogFinalFactor = clamp(exp(-FogGlobalDensity * fogDistInt * fogHeightInt),0,1);

	// Find the sun highlight and use it to blend the fog color
	float sunHighlightFactor = saturate(dot(eyeToPixelNorm, FogSunDir));
	sunHighlightFactor = pow(sunHighlightFactor, 8.0);
	float3 fogFinalColor = lerp(FogColor, FogHighlightColor, sunHighlightFactor);
	return lerp(fogFinalColor, originalColor, fogFinalFactor);
}

float4 main(
	PS_INPUT IN,
	uniform float v1 : register(c0),
	uniform float v2,
	uniform float v3,
	uniform float v4,
	uniform matrix iview,
	uniform float3 campos,
	uniform float3 viewdir
) : SV_Target
{
	float4 dbuffer = 1.0-rt_depth.Sample(SS_depth, IN.uv0);

	float red 	= 0.2;
	float green = 0.4;
	float blue 	= 0.6;
	float4 fogColour = float4(red, green, blue, 1);

	// // get the world pos
	float4 wpos = mul(iview, dbuffer); //gl_Position
	wpos /= wpos.w;
	// float dist = length(campos - wpos.xyz);

	// float viewer 		= campos.y;
	// float fragmentHeight = wpos.y;
	// float heightBias 	= v1 * 10;
	// float density 		= v2 * 0.01;
	// float verticalDecay = v3 * 0.2;
	// float baseLevel 	= v4 * 500;
	// float fog = (ExpGroundFog(dist, viewer, fragmentHeight - heightBias, density, verticalDecay, bas	eLevel);

	float4 scene = rt_scene.Sample(SS_scene, IN.uv0);
	// float4 result = lerp(scene, fogColour, fog);




// vec3 applyFog( in vec3  rgb,      // original color of the pixel
//                in float distance, // camera to point distance
//                in vec3  rayOri,   // camera position
//                in vec3  rayDir )  // camera to point vector
	float4 meh = mul(iview, viewdir); //gl_Position

	float dist = length(meh - campos.xyz);
	float b =v1;
	float c =v2;
	float fogAmount = saturate(c * exp(-campos.y * b) * (1.0 - exp(-dist * viewdir.y * b)) / viewdir.y);
	fogAmount = fogAmount;
	float4 fogColor = float4(0.5, 0.6, 0.7, 1);
	float4 result = lerp(scene, fogColor, fogAmount);

	// float4 result = float4(ApplyFog(scene.xyz, campos.y, dist).xyz, 1);
	// float4 result = saturate(wpos.yyyw);
	return result;
}

