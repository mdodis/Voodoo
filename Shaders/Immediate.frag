#version 450

layout (location = 0 ) in vec3 inColor;

//output write
layout (location = 0) out vec4 outFragColor;

void main()
{
    vec3 color = inColor;
    outFragColor = vec4(color, 1.0f);
}