#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout (location = 0) out vec3 outColor;

layout (set = 0, binding = 0) uniform CameraBuffer {
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} cameraData;

struct GPUObjectData {
	mat4 model;
};

layout (std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
	GPUObjectData objects[];
} objectBuffer;

void main()
{
	mat4 modelMatrix = objectBuffer.objects[gl_BaseInstance].model;
	mat4 transform = cameraData.viewproj * modelMatrix;
	// mat4 transform = cameraData.proj * modelMatrix;
	gl_Position = transform * vec4(vPosition, 1.0f);
	outColor = vColor;
}