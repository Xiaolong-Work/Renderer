/*****************************************************************//**
 * @file   pbr.glsl
 * @brief  PBR Shading Calculation
 * 
 * @author Xiaolong
 * @date   April 2025
 *********************************************************************/

#include "light.glsl"

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


vec4 shaderPointLightPBR(PointLight point_light, )
{
	vec3 light_position = light.position;
	vec3 shading_position = position;
	vec3 eye_position = vec3(0);
}

vec4 shaderPBR(vec3 radiance, vec3 light_direction ,float roughness, float metallic, vec4 shader_color, vec3 position, vec3 normal)
{
	vec3 light_position = light.position;
	vec3 shading_position = position;
	vec3 eye_position = vec3(0);

	vec3 l = normalize(light_position - shading_position);
	vec3 v = normalize(eye_position - shading_position);
	vec3 n = normalize(normal);
	vec3 h = normalize(v + l);

	vec3 r0 = mix(vec3(0.04), vec3(shader_color), metallic);

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