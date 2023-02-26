Texture2D RT_depth				: register(t0);
SamplerState samplerState_depth	: register(s0);
Texture2D RT_scene				: register(t1);
SamplerState samplerState_scene	: register(s1);

// // use this with lightrays ;)
// float4 toBlack(float4 c, float u)
// {
// 	//-------------- black for lightrays
// 	float bw = 1.0;
// 	if(u>0.4){ 	bw=0; }else{bw=1;};
// 	c = lerp(float4(0,0,0,1), c, bw);
// 	//----------------------------------
// 	return c;
// }

float4 Blur1(float2 uv, float d, float scale){
	float w = 2048;
	float h = 1024;
	float sz = scale * (1.0 - d);
	float4 s11 = RT_scene.Sample(samplerState_scene, uv + float2(-sz / w, -sz / h)); // LEFT
	float4 s12 = RT_scene.Sample(samplerState_scene, uv + float2(0, -sz / h));				// MIDDLE
	float4 s13 = RT_scene.Sample(samplerState_scene, uv + float2(sz / w, -sz / h));	// RIGHT
	// MIDDLE ROw
	float4 s21 = RT_scene.Sample(samplerState_scene, uv + float2(-sz / w, 0)); // LEFT
	float4 col = RT_scene.Sample(samplerState_scene, uv);						// DEAD CENTER
	float4 s23 = RT_scene.Sample(samplerState_scene, uv + float2(-sz / w, 0)); // RIGHT
	// LAST ROw
	float4 s31 = RT_scene.Sample(samplerState_scene, uv + float2(-sz / w, sz / h)); // LEFT
	float4 s32 = RT_scene.Sample(samplerState_scene, uv + float2(0, sz / h));			 // MIDDLE
	float4 s33 = RT_scene.Sample(samplerState_scene, uv + float2(sz / w, sz / h));	// RIGHT
	col = (col + s11 + s12 + s13 + s21 + s23 + s31 + s32 + s33) / 9;
	return col;
}


static const float GOLDEN_ANGLE = 2.39996;
static const float ITERATIONS = 50;

//-------------------------------------------------------------------------------------------
float3 Blur_Bokeh(float2 uv, float radius, float db){
	float2x2 rot = float2x2(
		cos(GOLDEN_ANGLE), sin(GOLDEN_ANGLE), //row 1
		-sin(GOLDEN_ANGLE), cos(GOLDEN_ANGLE) // row 2
	);
	float3 acc = float3(0.0, 0.0, 0.0 );
	float r = 1.0;
	float2 vangle = float2(0.0, radius * 0.01 / sqrt(float(ITERATIONS)))*db;

	for (int j = 0; j < ITERATIONS; ++j)
	{
	 	// the approx increase in the scale of sqrt(0, 1, 2, 3...)
		r = r + (1.0 / r);
		vangle = mul(vangle, rot);
		float3 col = RT_scene.Sample(samplerState_scene, uv + ((r - 1.0) * vangle) * db).xyz;
		
		float3 bokeh = pow(col, 1.0); //brightness of effect
		acc += bokeh;
	}
	
	float3 finalblur = acc / ITERATIONS;
	return finalblur;
}

float4 main(
	float2 uv0: TEXCOORD0,
	  uniform float v1,
	  uniform float v2,
	  uniform float v3
	  ) : SV_Target
{
	float range = v1; //30.0;
	float near = v2;
	// float focus = v2; //1.016;

	float depth = 1.0-RT_depth.Sample(samplerState_depth, uv0).x;
	//float4 scene = RT_scene.Sample(samplerState_scene, uv0).xyzw;

	// float _buffer = saturate(range * abs(focus - depth) + v4);
	float buffer = saturate(1 - ((2 * near) / (range + near - depth * (range - near)))); // better linear equation

	//Basic---------------------------------
	// float4 col = Blur1(uv0, buffer, v3);


	//??---------------------------------
	// float4 col = blur3(uv0,v3);

	//Bokeh---------------------------------
	float3 blur_0 = Blur_Bokeh(uv0, v3, 1-buffer);
	float4 col = float4(blur_0, 1);
	//float4 output2 = lerp(col, scene, buffer);
	return col;
}
