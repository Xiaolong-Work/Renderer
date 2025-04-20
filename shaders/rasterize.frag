#version 450 core
#extension GL_EXT_nonuniform_qualifier : enable


layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture_coordinate;
layout(location = 3) in vec4 color;
layout(location = 4) flat in int draw_id;
layout(location = 5) flat in vec3 camera_position;


layout(location = 0) out vec4 out_color;

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

struct PointLight
{
	vec3 position;
	vec3 color;
	float intensity;
};

layout(binding = 2) uniform sampler2D textures[]; 
layout(binding = 3) uniform samplerCubeArray shadow_map;
layout(std430, binding = 4) readonly buffer PointLightBuffer { PointLight point_lights[]; };
layout(std430, binding = 5) readonly buffer IndexBuffer { int material_indices[]; };
layout(std430, binding = 6) readonly buffer MaterialBuffer { Material materials[]; };

float shadow(int i)
{
	vec3 shading_position = vec3(position / position.w);

	vec3 light_position = point_lights[i].position;
    vec3 light_to_shading_point = shading_position - light_position;
    float distance = length(light_to_shading_point) / 100.0f;
    vec3 direction = normalize(light_to_shading_point);

	float min_bias = 0.001;
	float max_biax = 0.005;
	float bias = max(max_biax * (1.0 - dot(normal, -direction)), min_bias);
    
	float d = texture(shadow_map, vec4(direction, float(i))).r;

	if (d > distance - bias) return 1.0f;
    return 0.05f;
}

vec4 shader(PointLight light, Material material, vec3 kd_color, vec3 ks_color)
{
	vec3 light_position = light.position;
	vec3 eye_position = camera_position;
	vec3 shading_position = vec3(position / position.w);

	vec3 l = normalize(light_position - shading_position);
	vec3 v = normalize(eye_position - shading_position);
	vec3 n = normalize(normal);
	vec3 h = normalize(v + l);

	float r2 = dot((light_position - shading_position), (light_position - shading_position));
	vec3 radiance = light.color / r2;
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

vec4 shaderPBR(PointLight light, Material material, vec4 shader_color)
{
	vec3 light_position = light.position;
	vec3 eye_position = camera_position;
	vec3 shading_position = vec3(position / position.w);

	vec3 l = normalize(light_position - shading_position);
	vec3 v = normalize(eye_position - shading_position);
	vec3 n = normalize(normal);
	vec3 h = normalize(v + l);

	float intensity = light.intensity;  // lumens
	vec3 light_color = light.color;
	float r2 = dot((light_position - shading_position), (light_position - shading_position));
	float irradiance = intensity / (4.0 * 3.14159265 * r2);
	vec3 radiance = irradiance * light_color;
	
	vec3 r0 = mix(vec3(0.04),vec3(shader_color), material.metallic);
	
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
	vec3 diffuse = (vec3(shader_color) / 3.14159265) * kD;
	
	return vec4((diffuse + specular) * radiance * NdotL, 1.0f);
}

void main1()
{
	out_color = vec4(1, 0, 0,1);
}

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
		shader_color = texture(textures[texture_index], texture_coordinate);
	}
	shader_color *= color;
	
	if (shader_color.a < 0.5) 
	{
		discard;
	}
	
	vec4 result_color = vec4(0.0f);
	for (int i = 0; i < point_lights.length(); i++)
	{
		float shadow_factor = shadow(i);
		result_color += shaderPBR(point_lights[i], materials[material_index], shader_color) * shadow_factor;
	}
	
	out_color = result_color;
	//out_color = vec4((normalize(normal) + vec3(1.0f, 1.0f, 1.0f)) / 2.0f, 1.0f);
}	