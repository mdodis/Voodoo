#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout (location = 0) out vec3 outColor;

layout (set = 0, binding = 0) uniform CameraBuffer {
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} cameraData;

//push constants block
layout( push_constant ) uniform constants
{
	vec4 data;
	mat4 render_matrix;
} pushConstants;

void main()
{
	mat4 transform = cameraData.viewproj * pushConstants.render_matrix;
	gl_Position = transform * vec4(vPosition, 1.0f);
	outColor = vColor;
}