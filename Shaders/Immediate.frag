#version 450

//output write
layout (location = 0) out vec4 outFragColor;

struct GPUCameraData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
};

struct GPUSceneData {
    vec4 fog_color;
    vec4 ambient_color;
};

layout (set = 0, binding = 0) uniform GPUGlobalInstanceData {
    GPUCameraData camera;
    GPUSceneData  scene;
} globalData;

void main()
{
    vec3 color = vec3(1, 0, 0);
    outFragColor = vec4(color, 1.0f);
}