#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 world_position;
layout(location = 1) flat in vec3 light_position;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 texture_coordinate;
layout(location = 4) flat in int draw_id;
layout(location = 5) flat in int type;

layout(location = 0) out float liner_depth;

#define POINT 0
#define DIRECTION 1

struct Material
{
	/* ========== Blinn-Phong ========== */
	vec3 ka;
	float ns;

	vec3 kd;
	int diffuse_texture;

	vec3 ks;
	int specular_texture;

	vec3 tr;
	float ni;
	
	/* ========== PBR ========== */
	vec4 albedo;
	int albedo_texture;
	float metallic;
	float roughness;

	int type;
};
layout(binding = 2) uniform sampler2D textures[]; 
layout(std430, binding = 3) readonly buffer IndexBuffer { int material_indices[]; };
layout(std430, binding = 4) readonly buffer MaterialBuffer { Material materials[]; };

void main() 
{
	int material_index = material_indices[draw_id];
	Material material = materials[material_index];

	int texture_index = material.albedo_texture;

	vec4 shader_color;
	if (texture_index == -1)
	{
		shader_color = material.albedo;
	}
	else
	{
		shader_color = texture(textures[1], texture_coordinate);
	}

	if (shader_color.a < 0.5) 
	{
		discard;
	}
	else
	{
		if (type == POINT)
		{
			liner_depth = length(world_position - light_position) / 100.0f;
		}
		else if (type == DIRECTION)
		{
			liner_depth = 0;
		}
	}
}