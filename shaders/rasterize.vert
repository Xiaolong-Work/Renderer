#version 450
#extension GL_ARB_shader_draw_parameters : enable

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 model;
    mat4 view;
    mat4 project;
} ubo;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texture_coordinate;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec2 out_texture_coordinate;
layout(location = 2) out int draw_id;

void main() 
{
    gl_Position = ubo.project * ubo.view * ubo.model * vec4(in_position, 1.0);

    out_normal = in_normal;
    out_texture_coordinate = in_texture_coordinate;
	draw_id = gl_DrawIDARB;
}
