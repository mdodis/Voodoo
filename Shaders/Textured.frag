#version 450

//shader input
layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inTexCoord;

//output write
layout (location = 0) out vec4 outFragColor;

layout (set = 2, binding = 0) uniform sampler2D tex1;


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
    vec3 color = texture(tex1, inTexCoord).xyz;
    outFragColor = vec4(color, 1.0f);
}