#version 450 core
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec4 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texture_coordinate;
layout(location = 3) in vec4 in_color;
layout(location = 4) flat in int draw_id;

layout(location = 0) out vec4 out_position;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_color;
layout(location = 3) out vec4 out_material;

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
		shader_color = texture(textures[texture_index], in_texture_coordinate);
	}
	shader_color *= in_color;

	if (shader_color.a < 0.5) 
	{
		discard;
	}

	out_position = in_position;
	out_normal = vec4(in_normal, 1.0f);
	out_color = shader_color;
	out_material = vec4(material.metallic, material.roughness, 1.0f, 1.0f);
}	
