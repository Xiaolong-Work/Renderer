/*****************************************************************//**
 * @file   pbr.glsl
 * @brief  PBR Shading Calculation
 * 
 * @author Xiaolong
 * @date   April 2025
 *********************************************************************/

#include "light.glsl"
#include "material.glsl"

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

vec3 sampleGGXVNDF(vec3 normal, vec3 wi, float roughness, float rand1, float rand2)
{
	// 变换粗糙度到半角方向空间
	float a = roughness * roughness;

	// 坐标系
	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangentX = normalize(cross(up, normal));
	vec3 tangentY = cross(normal, tangentX);

	// 坐标系中采样
	float phi = 2.0 * 3.14159265 * rand1;
	float cosTheta = sqrt((1.0 - rand2) / (1.0 + (a * a - 1.0) * rand2));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	vec3 h = cos(phi) * sinTheta * tangentX + sin(phi) * sinTheta * tangentY + cosTheta * normal;
	vec3 result = reflect(-wi, h);

	return normalize(result);
}


vec3 shaderPBR(vec3 wi, vec3 wo, float roughness, float metallic, vec4 color, vec3 normal)
{
	vec3 l = normalize(wo);
	vec3 v = normalize(wi);
	vec3 n = normalize(normal);
	vec3 h = normalize(v + l);

	vec3 r0 = mix(vec3(0.04), vec3(color), metallic);

	vec3 f = fresnelSchlick(max(dot(h, v), 0.0), r0);
	float d = distributionGGX(n, h, roughness);
	float g = geometrySmith(n, v, l, roughness);

	vec3 numerator = d * g * f;
	float denominator = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0) + 0.0001;
	vec3 specular = numerator / denominator;

	vec3 ks = f;
	vec3 kd = vec3(1.0) - ks;
	kd *= 1.0 - metallic;

	float NdotL = max(dot(n, l), 0.0);
	vec3 diffuse = (vec3(color) / 3.14159265) * kd;

	return diffuse + specular;  
}

vec3 shaderPointLightPBR(vec3 position, vec4 color, vec3 normal, vec3 camera_position, PointLight light,  Material material)
{
	vec3 light_position = light.position;
	float intensity = light.intensity;
	vec3 light_color = light.color;

	return shaderPBR(camera_position - position, light_position - position, material.roughness, material.metallic, color, normal);
}