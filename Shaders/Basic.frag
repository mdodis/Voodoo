#version 450

//shader input
layout (location = 0) in vec3 inColor;

//output write
layout (location = 0) out vec4 outFragColor;


layout (set = 0, binding = 1) uniform SceneData {
	vec4 fog_color;
	vec4 ambient_color;
} sceneData;

void main()
{

	outFragColor = vec4(inColor + sceneData.ambient_color.xyz, 1.0f);
}