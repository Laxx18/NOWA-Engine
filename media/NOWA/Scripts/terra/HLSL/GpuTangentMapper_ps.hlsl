// Our terrain has the following pattern:
//
//	 1N  10   11
//		o-o-o
//		|/|/|
//	 0N	o-+-o 01
//		|/|/|
//		o-o-o
//	 NN  N0   N1
//
// We need to calculate the normal of the vertex in
// the center '+', which is shared by 6 triangles.

Texture2D<float> heightMap;

struct PS_INPUT
{
	float2 uv0 : TEXCOORD0;
};

float4 main
(
	PS_INPUT inPs,
	uniform float2 heightMapResolution
) : SV_Target
{
	int2 iCoord = int2( inPs.uv0 * heightMapResolution );

	int x = iCoord.x;
	int y = iCoord.y;

	float3 a, b, c;

	/////NOTE! texure x/y is reversed for load - ??

	c = float3(x, heightMap.Load(int3(x, y,0 )),		y);

	if (x == heightMapResolution.x - 1)
		b = float3( x-1, heightMap.Load(int3(x-1, y,	0)),	y);
	else
		b = float3(x + 1, heightMap.Load(int3(x + 1, y, 0)),	y) ;

	if (y == heightMapResolution.y - 1)
		a = float3(x, heightMap.Load(int3(x,y - 1, 0)),		 y-1);
	else
		a = float3(x, heightMap.Load(int3( x,y + 1, 0 )),	y+1) ;

	float3 t = a - c;

	t = normalize(t);
	
	return float4(t.zyx * 0.5f + 0.5f, 1.0f);
}
