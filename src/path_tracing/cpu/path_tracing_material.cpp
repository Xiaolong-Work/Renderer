#include <path_tracing_material.h>

PathTracingMaterial::PathTracingMaterial(Material& material)
{
	this->name = std::move(material.name);
	this->Ka = std::move(material.Ka);
	this->Kd = std::move(material.Kd);
	this->Ks = std::move(material.Ks);
	this->Tr = std::move(material.Tr);
	this->Ns = std::move(material.Ns);
	this->Ni = std::move(material.Ni);
	this->map_Kd = std::move(material.map_Kd);
	this->type = std::move(material.type);
	this->texture_load_flag = std::move(material.texture_load_flag);
	this->texture = std::move(material.texture);
}

Vector3f PathTracingMaterial::sample(const Direction& wi, const Direction& normal) const
{
	if (this->type == MaterialType::Glossy)
	{
		return glossySample(wi, normal);
	}
	else if (this->type == MaterialType::Specular)
	{
		return reflectSample(wi, normal);
	}
	else if (this->type == MaterialType::Refraction)
	{
		return refractionSample(wi, normal, this->Ni);
	}
	else
	{
		return diffuseSample(normal);
	}
}

Vector3f PathTracingMaterial::glossySample(const Direction& wi, const Direction& normal) const
{
	/* Calculate the reflection direction */
	Direction reflect_direction = glm::reflect(wi, normal);

	/* Generate random numbers */
	float u = getRandomNumber(0.0f, 1.0f);
	float v = getRandomNumber(0.0f, 1.0f);

	/* Use Phong cosine distribution for importance sampling */
	float exponent = this->Ns;							  // Specular Index
	float cos_theta = std::pow(u, 1.0f / (exponent + 1)); // Cosine sampling
	float sin_theta = std::sqrt(1 - cos_theta * cos_theta);
	float phi = 2.0f * pi * v; // Uniform azimuth distribution

	/* Construct random offset in the local coordinate system in the reflection direction */
	float x = sin_theta * std::cos(phi);
	float y = sin_theta * std::sin(phi);
	float z = cos_theta;

	/* Construct an orthogonal basis */
	Vector3f tangent, bitangent;
	if (std::abs(reflect_direction.z) > 0.999f)
	{
		tangent = Vector3f(1, 0, 0);
	}
	else
	{
		tangent = glm::normalize(glm::cross(reflect_direction, Vector3f(0, 0, 1)));
	}
	bitangent = glm::cross(reflect_direction, tangent);

	/* Transform to world coordinates */
	return glm::normalize(tangent * x + bitangent * y + reflect_direction * z);
}

Vector3f PathTracingMaterial::diffuseSample(const Direction& normal) const
{
	/* Generate random numbers */
	float u = getRandomNumber(0.0f, 1.0f);
	float v = getRandomNumber(0.0f, 1.0f);

	/* Cosine-weighted solid angle distribution */
	float phi = 2 * pi * u;
	float cos_theta = std::sqrt(v);
	float sin_theta = std::sqrt(1 - v);
	float x = std::cos(phi) * sin_theta;
	float y = std::sin(phi) * sin_theta;
	float z = cos_theta;

	/* Calculate the local coordinate system */
	Vector3f tangent, bitangent;
	if (std::abs(normal.z) > 0.999f)
	{
		tangent = Vector3f(1, 0, 0);
	}
	else
	{
		tangent = glm::normalize(glm::cross(normal, Vector3f(0, 0, 1)));
	}
	bitangent = glm::cross(normal, tangent);

	/* Transform to world coordinates */
	return tangent * x + bitangent * y + normal * z;
}

Vector3f PathTracingMaterial::reflectSample(const Direction& wi, const Direction& normal) const
{
	return glm::reflect(wi, normal);
}

Vector3f PathTracingMaterial::refractionSample(const Direction& wi, const Direction& normal, const float ni) const
{
	float cos_theta_i = glm::dot(-wi, normal);
	/* Ensure the media refractive index is correct */
	float eta = (cos_theta_i > 0) ? (1.0f / ni) : ni;
	Direction n = (cos_theta_i > 0) ? normal : -normal;
	cos_theta_i = std::abs(cos_theta_i);

	float discriminant = 1.0f - eta * eta * (1.0f - cos_theta_i * cos_theta_i);

	/* Total internal reflection occurs */
	if (discriminant < 0)
	{
		return glm::reflect(wi, normal);
	}
	else
	{
		/* Randomly select reflection or refraction based on the Fresnel term */
		float p = getRandomNumber(0.0f, 1.0f);
		if (p < fresnel(wi, normal, ni))
		{
			return glm::reflect(wi, normal);
		}
		else
		{
			float cosThetaT = std::sqrt(discriminant);
			return glm::normalize(eta * (-wi) + (eta * cos_theta_i - cosThetaT) * n);
		}
	}
}

float PathTracingMaterial::pdf(const Vector3f& wi, const Vector3f& wo, const Vector3f& normal) const
{
	if (this->type == MaterialType::Glossy)
	{
		return 1.0f / (2.0f * pi) * (1.0f / this->Ns) + 1.0f * (1.0f - 1.0f / this->Ns);
	}
	else if (this->type == MaterialType::Refraction || this->type == MaterialType::Specular)
	{
		return 1.0f;
	}
	else if (this->type == MaterialType::Diffuse)
	{
		return 1.0f / (2.0f * pi);
	}
	else
	{
		return 0.0f;
	}
}

Vector3f PathTracingMaterial::evaluate(const Vector3f& wi,
									   const Vector3f& wo,
									   const Vector3f& normal,
									   const Vector3f& color) const
{
	if (this->type == MaterialType::Diffuse)
	{
		if (glm::dot(normal, wo) <= 0.0f)
		{
			return Vector3f{0.0f};
		}
		else
		{
			if (this->texture_load_flag)
			{
				return color * Kd / glm::pi<float>();
			}
			else
			{
				return this->Kd / glm::pi<float>();
			}
		}
	}
	else if (this->type == MaterialType::Glossy)
	{
		Vector3f diffuse = std::max(glm::dot(normal, wo), 0.0f) * this->Kd;
		;

		if (this->texture_load_flag)
		{
			diffuse = Vector3f{std::powf(color.x / 255.0f, 1.0f / 0.6f),
							   std::powf(color.y / 255.0f, 1.0f / 0.6f),
							   std::powf(color.z / 255.0f, 1.0f / 0.6f)} *
					  std::max(glm::dot(normal, wo), 0.0f) / pi;
			if (this->Kd != Vector3f{0.0f, 0.0f, 0.0f})
			{
				diffuse *= this->Kd;
			}
		}

		Direction half_direction = glm::normalize(wi + wo);
		Vector3f specular = std::pow(std::max(glm::dot(half_direction, normal), 0.0f), this->Ns) * this->Ks;
		return diffuse + specular + Ka;
	}
	else if (this->type == MaterialType::Specular)
	{
		Direction half_direction = glm::normalize(wi + wo);
		return std::pow(std::max(glm::dot(half_direction, normal), 0.0f), this->Ns) * this->Ks + Ka;
	}
	else if (this->type == MaterialType::Refraction)
	{
		return this->Kd / pi + Ka;
	}
	else
	{
		return this->Kd / pi + Ka;
	}
}

float PathTracingMaterial::fresnel(const Vector3f& wi, const Vector3f& normal, const float& ni) const
{
	float cos_i = clamp(-1.0f, 1.0f, glm::dot(wi, normal));
	float eta_i = 1, eta_t = ni;
	if (cos_i > 0)
	{
		std::swap(eta_i, eta_t);
	}

	float sin_t = eta_i / eta_t * std::sqrtf(std::max(0.0f, 1 - cos_i * cos_i));

	if (sin_t >= 1)
	{
		return 1.0f;
	}
	else
	{
		float cos_t = sqrtf(std::max(0.f, 1 - sin_t * sin_t));
		cos_i = fabsf(cos_i);
		float Rs = ((eta_t * cos_i) - (eta_i * cos_t)) / ((eta_t * cos_i) + (eta_i * cos_t));
		float Rp = ((eta_i * cos_i) - (eta_t * cos_t)) / ((eta_i * cos_i) + (eta_t * cos_t));
		return (Rs * Rs + Rp * Rp) / 2;
	}
}
