#version 450
#extension GL_KHR_vulkan_glsl : enable

float fresnel(vec3 wi, vec3 normal, float ni)
{
	float cos_i = clamp(-1.0f, 1.0f, dot(wi, normal));
	float eta_i = 1, eta_t = ni;
	if (cos_i > 0)
	{
		swap(eta_i, eta_t);
	}

	float sin_t = eta_i / eta_t * sqrt(max(0.0f, 1 - cos_i * cos_i));

	if (sin_t >= 1)
	{
		return 1.0f;
	}
	else
	{
		float cos_t = sqrt(max(0.f, 1 - sin_t * sin_t));
		cos_i = abs(cos_i);
		float Rs = ((eta_t * cos_i) - (eta_i * cos_t)) / ((eta_t * cos_i) + (eta_i * cos_t));
		float Rp = ((eta_i * cos_i) - (eta_t * cos_t)) / ((eta_i * cos_i) + (eta_t * cos_t));
		return (Rs * Rs + Rp * Rp) / 2;
	}
}

vec3 glossySample(vec3 wi, vec3 normal, Material material, inout uint random_seed)
{
	/* Compute the reflection direction and ensure normalization */
	vec3 reflect_direction = normalize(reflect(wi, normal));

	float u = rand(random_seed);
	float v = rand(random_seed);

	float exponent = max(material.Ns, 0.0f); // Ensure the exponent is non-negative
	float cos_theta = pow(max(u, 1e-6f), 1.0f / (exponent + 1));
	float sin_theta = sqrt(1 - cos_theta * cos_theta);
	float phi = 2.0f * pi * v;

	float x = sin_theta * cos(phi);
	float y = sin_theta * sin(phi);
	float z = cos_theta;

	// Construct an orthonormal basis around the reflection direction
	vec3 tangent = (abs(reflect_direction.z) > 0.999f) ? vec3(1, 0, 0)
														   : // If too close to the Z-axis, use (1,0,0) as tangent
						   normalize(cross(reflect_direction, vec3(0, 1, 0)));
	vec3 bitangent = normalize(cross(tangent, reflect_direction));

	// Transform the sampled direction from local to world space and return
	return normalize(tangent * x + bitangent * y + reflect_direction * z);
}

vec3 diffuseSample(vec3 normal, inout uint random_seed)
{
	/* Generate random numbers */
	float u = rand(random_seed);
	float v = rand(random_seed);

	/* Cosine-weighted solid angle distribution */
	float phi = 2 * pi * u;
	float cos_theta = sqrt(v);
	float sin_theta = sqrt(1 - v);
	float x = cos(phi) * sin_theta;
	float y = sin(phi) * sin_theta;
	float z = cos_theta;

	/* Calculate the local coordinate system */
	vec3 tangent, bitangent;
	if (abs(normal.z) > 0.999f)
	{
		tangent = vec3(1, 0, 0);
	}
	else
	{
		tangent = normalize(cross(normal, vec3(0, 0, 1)));
	}
	bitangent = cross(normal, tangent);

	/* Transform to world coordinates */
	return tangent * x + bitangent * y + normal * z;
}

vec3 reflectSample(vec3 wi, vec3 normal)
{
	return reflect(wi, normal);
}

vec3 refractionSample(vec3 wi, vec3 normal, float ni, inout uint random_seed)
{
	float cos_theta_i = dot(-wi, normal);
	float eta = 1.0f / ni;
	float discriminant = 1.0f - eta * eta * (1.0f - cos_theta_i * cos_theta_i);

	/* 发生全反射，折射光线不存在 */

	/* 根据菲涅尔项随机选择反射和折射 */
	float p = rand(random_seed);

	if (p < fresnel(wi, normal, ni))
	{
		return reflect(wi, normal);
	}
	else
	{
		float cosThetaT = sqrt(discriminant);
		return eta * wi + (eta * cos_theta_i - cosThetaT) * normal;
	}
}

#define Diffuse 0
#define Specular 1
#define Refraction 2
#define Glossy 3

float pdfMaterial(vec3 wi, vec3 wo, vec3 normal, Material material)
{
	if (material.type == Glossy)
	{
		return 1.0f / (2.0f * pi) * (1.0f / material.Ns) + 1.0f * (1.0f - 1.0f / material.Ns);
	}
	else if (material.type == Refraction || material.type == Specular)
	{
		return 1.0f;
	}
	else if (material.type == Diffuse)
	{
		return 1.0f / (2.0f * pi);
	}
	else
	{
		return 0.0f;
	}
}

vec3 evaluateMaterial(vec3 wi, vec3 wo, vec3 normal, vec3 color, Material material)
{
	if (material.type == Diffuse)
	{
		vec3 shader_color;
		if (material.texture_index != -1)
		{
			shader_color = color;
		}
		else
		{
			shader_color = material.Kd;
		}
		return max(dot(normal, wo), 0.0f) * shader_color / pi;
	}
	else if (material.type == Glossy)
	{
		vec3 shader_color;
		if (material.texture_index != -1)
		{
			shader_color = color;
		}
		else
		{
			shader_color = material.Kd;
		}

		vec3 diffuse = max(dot(normal, wo), 0.0f) * shader_color / pi;

		vec3 half_direction = normalize(wi + wo);
		vec3 specular = pow(max(dot(half_direction, normal), 0.0f), material.Ns) * material.Ks;

		return diffuse + specular;
	}
	else
	{
		return material.Kd / pi;
	}
}

vec3 sampleMaterial(vec3 wi, vec3 normal, Material material, inout uint random_seed)
{
	if (material.type == Glossy)
	{
		return glossySample(wi, normal, material, random_seed);
	}
	else if (material.type == Specular)
	{
		return reflectSample(wi, normal);
	}
	else if (material.type == Refraction)
	{
		return refractionSample(wi, normal, material.Ni, random_seed);
	}
	else
	{
		return diffuseSample(normal, random_seed);
	}
}