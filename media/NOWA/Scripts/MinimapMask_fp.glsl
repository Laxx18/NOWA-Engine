#version 330 core

uniform sampler2D MinimapRT;
in vec2 uv0;
out vec4 fragColor;

void main()
{
    vec2 center = vec2(0.5, 0.5);
    float radius = 0.5;

    float dist = distance(uv0, center);
    if (dist > radius)
    {
        discard;
    }

    vec4 color = texture(MinimapRT, uv0);
    fragColor = color;
}