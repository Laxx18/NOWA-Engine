#version 330 core
layout(set = 0, binding = 0) uniform Params {
    uniform vec4 geomData1;
    uniform vec4 geomData2;
} vulkanParams;

layout(binding = 1) uniform sampler2D tex1;
layout(binding = 2) uniform sampler2D tex2;
in vec2 uv0;
out vec4 fragColor;
void main() {
    vec2 uv = uv0;
    if (uv.x >= geomData1.x && uv.x < (geomData1.x + geomData1.z) &&
        uv.y >= geomData1.y && uv.y < (geomData1.y + geomData1.w)) {
        vec2 adjustedUV = (uv - vec2(geomData1.x, geomData1.y)) / vec2(geomData1.z, geomData1.w);
        fragColor = texture(tex1, adjustedUV);
        return;
    }
    if (uv.x >= geomData2.x && uv.x < (geomData2.x + geomData2.z) &&
        uv.y >= geomData2.y && uv.y < (geomData2.y + geomData2.w)) {
        vec2 adjustedUV = (uv - vec2(geomData2.x, geomData2.y)) / vec2(geomData2.z, geomData2.w);
        fragColor = texture(tex2, adjustedUV);
        return;
    }
    fragColor = vec4(0, 0, 0, 1);
}
