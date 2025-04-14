#version 450 core
#extension GL_EXT_nonuniform_qualifier : enable


layout(location = 0) in vec3 world_position;
layout(location = 1) in vec3 view_position;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 texture_coordinate;
layout(location = 4) flat in int draw_id;
layout(location = 5) flat in mat4 view;


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
	vec3 albedo;
	float metallic;
	float roughness;
};

struct PointLight
{
	vec3 position;
	vec3 intensity;
};

struct MVP
{
    mat4 model;
    mat4 view;
    mat4 project;
};

layout(binding = 1) uniform sampler2D textures[]; 
layout(binding = 2) uniform samplerCubeArray shadow_map;
layout(std430, binding = 3) readonly buffer PointLightBuffer { PointLight point_light[]; };
layout(std430, binding = 4) readonly buffer IndexBuffer { int material_indices[]; };
layout(std430, binding = 5) readonly buffer MaterialBuffer { Material materials[]; };
layout(std430, binding = 6) readonly buffer MVPBuffer { MVP point_mvps[]; };

float NonLinearDepth(float linearDepth, float near, float far)
{
    float z = (far + near - (2.0 * near * far) / linearDepth) / (far - near);
    return z * 0.5 + 0.5; 
}

float shadow(int i)
{
	vec3 light_position = point_light[i].position;
    vec3 light_to_shadering_point = world_position - light_position;
    float distance = length(light_to_shadering_point);
    vec3 direction = normalize(light_to_shadering_point);

    float bias = 3;

	float d = texture(shadow_map, vec4(direction, float(i))).r;

	if (d > distance - bias) return 1.0f;
    return 0.0f;
}

vec4 shader(PointLight light, Material material, vec3 kd_color, vec3 ks_color)
{
	vec3 view_light_position = vec3(view * vec4(light.position, 1.0));

	vec3 eye_pos = vec3(0);

	vec3 l = normalize(view_light_position - view_position);
	vec3 v = normalize(eye_pos - view_position);
	vec3 n = normalize(normal);
	vec3 h = normalize(v + l);

	float r2 = dot((view_light_position - view_position), (view_light_position - view_position));
	vec3 radiance = light.intensity / r2;
	vec3 ld = material.kd * radiance * max(0.0f, dot(n, l));
	vec3 ls = material.ks * radiance * pow(max(0.0f, dot(n, h)), material.ns);

	return vec4(ls + ld, 1.0f);
}

vec3 fresnelSchlick(float cos_theta, vec3 r0)
{
	return r0 + (1.0 - r0) * pow(1.0 - cos_theta, 5.0);
}

float distributionGGX(vec3 n, vec3 h, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float n_dot_h = max(dot(n, h), 0.0);
	float n_dot_h2 = n_dot_h * n_dot_h;

	float numerator = a2;
	float denominator = (n_dot_h2 * (a2 - 1.0) + 1.0);
	denominator = 3.14159265 * denominator * denominator;

	return numerator / denominator;
}

float geometrySchlickGGX(float n_dot_v, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;

	float denominator = n_dot_v * (1.0 - k) + k;
	return n_dot_v / denominator;
}

float geometrySmith(vec3 n, vec3 v, vec3 l, float roughness)
{
	float n_dot_v = max(dot(n, v), 0.0);
	float n_dot_l = max(dot(n, l), 0.0);
	float ggx1 = geometrySchlickGGX(n_dot_v, roughness);
	float ggx2 = geometrySchlickGGX(n_dot_l, roughness);
	return ggx1 * ggx2;
}

vec4 shaderPBR(PointLight light, Material material)
{
	vec3 view_light_position = vec3(view * vec4(light.position, 1.0));
	vec3 eye_pos = vec3(0);

	vec3 l = normalize(view_light_position - view_position);
	vec3 v = normalize(eye_pos - view_position);
	vec3 n = normalize(normal);
	vec3 h = normalize(v + l);

	float r2 = dot((view_light_position - view_position), (view_light_position - view_position));
	vec3 radiance = light.intensity / r2;
	
	vec3 r0 = mix(vec3(0.04), material.albedo, material.metallic);
	
	vec3 f = fresnelSchlick(max(dot(h, v), 0.0), r0);
	float d = distributionGGX(n, h, material.roughness);
	float g = geometrySmith(n, v, l, material.roughness);
	
	vec3 numerator = d * g * f;
	float denominator = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0) + 0.0001;
	vec3 specular = numerator / denominator;
	
	vec3 kS = f;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - material.metallic;
	
	float NdotL = max(dot(n, l), 0.0);
	vec3 diffuse = (material.albedo / 3.14159265) * kD;
	
	return vec4((diffuse + specular) * radiance * NdotL, 1.0f);
}

void main() 
{
	PointLight light = point_light[0];
	
	int material_index = material_indices[draw_id];
	int texture_index = materials[material_index].diffuse_texture;

	if (texture_index == -1)
	{
		float t = shadow(0);
		out_color = shaderPBR(light, materials[material_index]) * t;
	}
	else
	{
		out_color = texture(textures[texture_index], texture_coordinate);
	}	

	// out_color = vec4((normalize(normal) + vec3(1.0f, 1.0f, 1.0f)) / 2.0f, 1.0f);
}	