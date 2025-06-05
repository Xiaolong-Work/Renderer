/*****************************************************************//**
 * @file   pbr.glsl
 * @brief  PBR Shading Calculation
 * 
 * @author Xiaolong
 * @date   April 2025
 *********************************************************************/

#include "light.glsl"
#include "material.glsl"

vec3 fresnelSchlick(vec3 h, vec3 wi, vec3 r0)
{
	return r0 + (vec3(1.0) - r0) * pow(1.0 - max(dot(wi, h), 0), 5.0);
}

float fresnelSchlickIor(vec3 wi, vec3 normal, float ior)
{
	float r0 = (ior - 1.0) / (ior + 1.0);
	r0 = r0 * r0;
	return (fresnelSchlick(normal, wi, vec3(r0))).x;
}

float distributionGGX(vec3 n, vec3 h, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float n_dot_h = max(dot(n, h), 0.0);

	float numerator = a2;
	float denominator = n_dot_h * n_dot_h * (a2 - 1.0) + 1.0;
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


vec3 shaderPBR(vec3 wi, vec3 wo, float roughness, float metallic, vec4 color, vec3 normal)
{
	vec3 v = normalize(wi);
	vec3 l = normalize(wo);
	vec3 n = normalize(normal);
	vec3 h = normalize(v + l);

	vec3 r0 = mix(vec3(0.04), vec3(color), metallic);

	vec3 f = fresnelSchlick(h, v, r0);
	float d = distributionGGX(n, h, roughness);
	float g = geometrySmith(n, v, l, roughness);

	vec3 numerator = d * g * f;
	float denominator = 4.0 * max(dot(n, v), 1e-6) * max(dot(n, l), 1e-6);
	vec3 specular = numerator / denominator;

	vec3 ks = f;
	vec3 kd = vec3(1.0) - ks;
	kd *= 1.0 - metallic;

	vec3 diffuse = (vec3(color) / 3.14159265) * max(dot(n, l), 0.0) * kd;

	return  specular + diffuse;
}

vec3 shaderPointLightPBR(vec3 position, vec4 color, vec3 normal, vec3 camera_position, PointLight light,  Material material)
{
	vec3 light_position = light.position;
	float intensity = light.intensity;
	vec3 light_color = light.color;

	return shaderPBR(camera_position - position, light_position - position, material.roughness, material.metallic, color, normal);
}

