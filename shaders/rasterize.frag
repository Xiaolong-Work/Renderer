#version 450 core
#extension GL_EXT_nonuniform_qualifier : enable


layout(location = 0) in vec3 normal;
layout(location = 1) in vec2 texture_coordinate;
layout(location = 2) flat in int draw_id;

layout(location = 0) out vec4 out_color;

struct Material
{
	vec3 ka;
	vec3 kd;
	vec3 ks;
	vec3 tr;
	float ns;
	float ni;
	int diffuse_texture;
	int type;
};
layout(binding = 1) uniform sampler2D textures[]; 
layout(std430, binding = 2) readonly buffer IndexBuffer { int material_indices[]; };
layout(std430, binding = 3) readonly buffer MaterialBuffer { Material materials[]; };

void main() 
{
	int material_index = material_indices[draw_id];
	int texture_index = materials[material_index].diffuse_texture;

	if (texture_index == -1)
	{
		out_color = vec4(materials[material_index].kd, 1.0f);
	}
	else
	{
		out_color = texture(textures[texture_index], texture_coordinate);
	}    

	// out_color = vec4((normalize(normal) + vec3(1.0f, 1.0f, 1.0f)) / 2.0f, 1.0f);
}	