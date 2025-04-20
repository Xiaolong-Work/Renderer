#version 450
#extension GL_ARB_shader_draw_parameters : enable

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 model;
    mat4 view;
    mat4 project;
} ubo;

layout(binding = 1) readonly buffer ModelMatrix { mat4x4 models[]; };


layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texture_coordinate;
layout(location = 3) in vec4 in_color;

layout(location = 0) out vec4 out_position;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_texture_coordinate;
layout(location = 3) out vec4 out_color;
layout(location = 4) out int draw_id;
layout(location = 5) out vec3 camera_position;


void main() 
{
	mat4 model = models[gl_DrawIDARB];
	
	out_position = model * vec4(in_position, 1.0f);
    gl_Position = ubo.project * ubo.view * out_position;

	out_normal = normalize(transpose(inverse(mat3(model))) * in_normal);

    out_texture_coordinate = in_texture_coordinate;
	out_color = in_color;

	draw_id = gl_DrawIDARB;
	
	mat4 inverse_view = inverse(ubo.view);
    camera_position = vec3(inverse_view[3]); 
}
