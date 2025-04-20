#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) out vec4 out_color;

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput position;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput normal_depth;
layout(input_attachment_index = 2, set = 0, binding = 2) uniform subpassInput albedo;
layout(input_attachment_index = 3, set = 0, binding = 3) uniform subpassInput material_ssao;

struct PointLight
{
	vec3 position;
	vec3 color;
	float intensity;
};

layout(binding = 4) uniform samplerCubeArray shadow_map;
layout(std430, binding = 5) readonly buffer PointLightBuffer { PointLight point_lights[]; };


float shadow(int i, vec3 world_position, vec3 normal)
{
	
	vec3 shading_position = world_position;

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

vec4 shaderPBR(PointLight light, float roughness, float metallic, vec4 shader_color, vec3 world_position, vec3 normal)
{
	vec3 light_position = light.position;
	vec3 shading_position = world_position;
	vec3 eye_position = vec3(0);

	vec3 l = normalize(light_position - shading_position);
	vec3 v = normalize(eye_position - shading_position);
	vec3 n = normalize(normal);
	vec3 h = normalize(v + l);

	float intensity = light.intensity;
	vec3 light_color = light.color;
	float r2 = dot((light_position - shading_position), (light_position - shading_position));
	float irradiance = intensity / (4.0 * 3.14159265 * r2);
	vec3 radiance = irradiance * light_color;
	
	vec3 r0 = mix(vec3(0.04),vec3(shader_color), metallic);
	
	vec3 f = fresnelSchlick(max(dot(h, v), 0.0), r0);
	float d = distributionGGX(n, h, roughness);
	float g = geometrySmith(n, v, l, roughness);
	
	vec3 numerator = d * g * f;
	float denominator = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0) + 0.0001;
	vec3 specular = numerator / denominator;
	
	vec3 kS = f;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;
	
	float NdotL = max(dot(n, l), 0.0);
	vec3 diffuse = (vec3(shader_color) / 3.14159265) * kD;
	
	return vec4((diffuse + specular) * radiance * NdotL, 1.0f);
}

void main()
{
	vec4 world_position = subpassLoad(position);
	vec3 position = vec3(world_position / world_position.w);
	vec3 normal = subpassLoad(normal_depth).xyz;
	vec4 color = subpassLoad(albedo);
	float metallic = subpassLoad(material_ssao).x;
	float roughness = subpassLoad(material_ssao).y;
	float ssao = subpassLoad(material_ssao).z;

	vec4 result_color = vec4(0.0f);
	for (int i = 0; i < point_lights.length(); i++)
	{
		float shadow_factor = shadow(i, position, normal);
		result_color += shaderPBR(point_lights[i], roughness, metallic, color, position, normal) * shadow_factor;
	}

	out_color = result_color;
	// out_color = color + vec4(normal, 0.0f);
}