#version 450
#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_NV_viewport_array2 : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texture_coordinate;
layout(location = 3) in vec4 color;

layout(location = 0) out vec3 world_position;
layout(location = 1) out vec3 light_position;
layout(location = 2) out vec3 out_normal;
layout(location = 3) out vec2 out_texture_coordinate;
layout(location = 4) out int draw_id;
layout(location = 5) out int type;

#define POINT 0
#define DIRECTION 1

struct UniformBuffer
{
	mat4 model;
	mat4 view[6];
	mat4 project;
	vec3 position;
};

layout(std430, binding = 0) readonly buffer Buffer { UniformBuffer datas[]; };
layout(binding = 1) readonly buffer ModelMatrix { mat4x4 models[]; };


void main() 
{
	mat4x4 model = models[gl_DrawIDARB];
	world_position =  vec3(model * vec4(in_position, 1.0));

	gl_Layer = gl_InstanceIndex;
	draw_id = gl_DrawIDARB;
	out_texture_coordinate = in_texture_coordinate;

	UniformBuffer data = datas[gl_InstanceIndex];
	gl_Position = data.project * data.view[0] * model * vec4(in_position, 1.0);
	light_position = data.position;

	type = DIRECTION;
}