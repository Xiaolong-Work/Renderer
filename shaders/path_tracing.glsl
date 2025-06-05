#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
struct HitPayload
{
	vec3 hit_value;
	uint depth;
	uint seed;
	bool in_object;
	bool hit_light;
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

struct ObjectAddress
{
	uint64_t vertex_address;
	uint64_t index_address;
};

struct ObjectProperty
{
	vec3 radiance;
	int is_light;
	int triangle_count;
	float area;
	float pad1;
	float pad2;
};

