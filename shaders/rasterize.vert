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

layout(location = 0) out vec3 world_position;
layout(location = 1) out vec3 view_position;
layout(location = 2) out vec3 out_normal;
layout(location = 3) out vec2 out_texture_coordinate;
layout(location = 4) out int draw_id;
layout(location = 5) out mat4 out_view;


void main() 
{
	world_position = vec3(ubo.model * vec4(in_position, 1.0f));
	view_position = vec3(ubo.view * vec4(world_position, 1.0f));
	
    gl_Position = ubo.project * vec4(view_position, 1.0);

	mat3 normal_matrix = transpose(inverse(mat3(ubo.view * ubo.model)));
	out_normal = normalize(normal_matrix * in_normal);

    out_texture_coordinate = in_texture_coordinate;
	out_view = ubo.view;
	draw_id = gl_DrawIDARB;
}
