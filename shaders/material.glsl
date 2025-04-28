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

layout(std430, binding = 6) readonly buffer MaterialIndexBuffer
{
	int material_indices[];
};

layout(std430, binding = 7) readonly buffer MaterialBuffer
{
	Material materials[];
};
