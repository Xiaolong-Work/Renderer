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

struct Vectex
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

