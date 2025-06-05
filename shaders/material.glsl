#define Diffuse 0
#define Specular 1
#define Refraction 2
#define Glossy 3

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
	float pad;

	vec4 bottom_albedo;

	float bottom_albedo_texture;
	float bottom_metallic;
	float bottom_roughness;
	

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
