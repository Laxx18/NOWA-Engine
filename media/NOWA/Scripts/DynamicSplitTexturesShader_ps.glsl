#version 330 core
layout(set = 0, binding = 0) uniform Params {
    uniform vec4 geomData1;
    uniform vec4 geomData2;
	uniform vec4 geomData3;
    uniform vec4 geomData4;
} vulkanParams;

layout(binding = 1) uniform sampler2D tex1;
layout(binding = 2) uniform sampler2D tex2;
layout(binding = 3) uniform sampler2D tex3;
layout(binding = 4) uniform sampler2D tex4;

in vec2 uv0;
out vec4 fragColor;

void main() {
    vec2 uv = uv0;
	
	if (any(notEqual(geomData1, vec4(0.0, 0.0, 0.0, 0.0)))) {
		if (uv.x >= geomData1.x && uv.x < (geomData1.x + geomData1.z) &&
			uv.y >= geomData1.y && uv.y < (geomData1.y + geomData1.w)) {
			vec2 adjustedUV = (uv - vec2(geomData1.x, geomData1.y)) / vec2(geomData1.z, geomData1.w);
			fragColor = texture(tex1, adjustedUV);
			return;
		}
	}
	
	if (any(notEqual(geomData2, vec4(0.0, 0.0, 0.0, 0.0)))) {
		if (uv.x >= geomData2.x && uv.x < (geomData2.x + geomData2.z) &&
			uv.y >= geomData2.y && uv.y < (geomData2.y + geomData2.w)) {
			vec2 adjustedUV = (uv - vec2(geomData2.x, geomData2.y)) / vec2(geomData2.z, geomData2.w);
			fragColor = texture(tex2, adjustedUV);
			return;
		}
	}
	
	if (any(notEqual(geomData3, vec4(0.0, 0.0, 0.0, 0.0)))) {
		if (uv.x >= geomData3.x && uv.x < (geomData3.x + geomData3.z) &&
			uv.y >= geomData3.y && uv.y < (geomData3.y + geomData3.w)) {
			vec2 adjustedUV = (uv - vec2(geomData3.x, geomData3.y)) / vec2(geomData3.z, geomData3.w);
			fragColor = texture(tex3, adjustedUV);
			return;
		}
	}
	
	if (any(notEqual(geomData4, vec4(0.0, 0.0, 0.0, 0.0)))) {
		if (uv.x >= geomData4.x && uv.x < (geomData4.x + geomData4.z) &&
			uv.y >= geomData4.y && uv.y < (geomData4.y + geomData4.w)) {
			vec2 adjustedUV = (uv - vec2(geomData4.x, geomData4.y)) / vec2(geomData4.z, geomData4.w);
			fragColor = texture(tex4, adjustedUV);
			return;
		}
	}
	
    fragColor = vec4(0, 0, 0, 1);
}
