#version 450
#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_multiview : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texture_coordinate;

layout(location = 0) out vec3 world_position;
layout(location = 1) out vec3 light_position;

struct UniformBuffer
{
    mat4 model;
    mat4 view[6];
    mat4 project;
	vec3 position;
};

layout(std430, binding = 0) readonly buffer Buffer { UniformBuffer datas[]; };

void main() 
{
	UniformBuffer data = datas[gl_InstanceIndex];

	light_position = data.position;
	world_position =  vec3(data.model * vec4(in_position, 1.0));

	gl_Position = data.project * data.view[gl_ViewIndex] * data.model * vec4(in_position, 1.0);
}