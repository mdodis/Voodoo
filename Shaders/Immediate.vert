#version 460

layout (location = 0) in vec3 vPosition;

layout (location = 0) out vec3 vColor;

struct GPUObjectData {
    mat4 model;
    vec3 color;
};

struct GPUCameraData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
};

layout (set = 0, binding = 0) uniform GPUGlobalInstanceData {
    GPUCameraData camera;
} globalData;

layout (std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
    GPUObjectData objects[];
} objectBuffer;

void main()
{
    mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model;
    mat4 transform = globalData.camera.viewproj * modelMatrix;

    vColor = objectBuffer.objects[gl_BaseInstance].color;

    gl_Position = transform * vec4(vPosition, 1.0f);
}