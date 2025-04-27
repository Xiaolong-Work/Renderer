/*****************************************************************//**
 * @file   path_tracing.glsl
 * @brief  Ray tracing shader input and output definitions.
 * 
 * @author Xiaolong
 * @date   April 2025
 *********************************************************************/

struct HitPayload
{
	vec3 hit_value;
	uint seed;
};

struct PointLight
{
	vec3 position;
	vec3 color;
	float intensity;
};

struct ShadowPayload
{
	bool is_hit;
	uint seed;
};

struct Camera
{
	vec3 position;
	vec3 look;
	vec3 up;
	float fov;
};

struct Vertex
{
	/* The position of the vertex in 3D space. */
	vec3 position;

	/* The normal vector at the vertex, used for shading calculations. */
	vec3 normal;

	/* The 2D texture coordinates associated with the vertex.*/
	vec2 texture;

	/* The color of the vertex */
	vec4 color;
};

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

struct ObjectAddress
{
	uint64_t vertex_address;		   // Address of the Vertex buffer
	uint64_t index_address;		   // Address of the index buffer
};

