#version 450

layout (location = 0 ) in vec2 uv;

//output write
layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 0) uniform sampler2D tex;

void main()
{
    vec3 color = texture(tex, uv).xyz;

    outFragColor = vec4(color, 1.0f);
}