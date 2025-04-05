#include <path_tracing_material.h>

PathTracingMaterial::PathTracingMaterial(Material& material)
{
	this->name = std::move(material.name);
	this->ka = std::move(material.ka);
	this->kd = std::move(material.kd);
	this->ks = std::move(material.ks);
	this->tr = std::move(material.tr);
	this->ns = std::move(material.ns);
	this->ni = std::move(material.ni);
	this->type = std::move(material.type);
}

Vector3f PathTracingMaterial::sample(const Direction& wi, const Direction& normal) const
{
	if (this->type == MaterialType::Glossy)
	{
		return glossySample(-wi, normal);
	}
	else if (this->type == MaterialType::Specular)
	{
		return reflectSample(-wi, normal);
	}
	else if (this->type == MaterialType::Refraction)
	{
		return refractionSample(-wi, normal, this->ni);
	}
	else
	{
		return diffuseSample(normal);
	}
}

Vector3f PathTracingMaterial::glossySample(const Direction& wi, const Direction& normal) const
{
	// Compute the reflection direction and ensure normalization
	Direction reflect_direction = glm::normalize(glm::reflect(wi, normal));

	// Generate random numbers for sampling
	float u = getRandomNumber(0.0f, 1.0f);
	float v = getRandomNumber(0.0f, 1.0f);

	// Compute Phong cosine-weighted sampling
	float exponent = std::max(this->ns, 0.0f); // Ensure the exponent is non-negative
	float cos_theta = std::pow(std::max(u, 1e-6f), 1.0f / (exponent + 1));
	float sin_theta = std::sqrt(1 - cos_theta * cos_theta);
	float phi = 2.0f * pi * v;

	// Convert to local coordinate system
	float x = sin_theta * std::cos(phi);
	float y = sin_theta * std::sin(phi);
	float z = cos_theta;

	// Construct an orthonormal basis around the reflection direction
	Vector3f tangent = (std::abs(reflect_direction.z) > 0.999f) ? Vector3f(1, 0, 0)
																: // If too close to the Z-axis, use (1,0,0) as tangent
						   glm::normalize(glm::cross(reflect_direction, Vector3f(0, 1, 0)));
	Vector3f bitangent = glm::normalize(glm::cross(tangent, reflect_direction));

	// Transform the sampled direction from local to world space and return
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

/* Note : The direction of wi points to the shader */
Vector3f PathTracingMaterial::reflectSample(const Direction& wi, const Direction& normal) const
{
	return glm::reflect(wi, normal);
}

/* Note : The direction of wi points to the shader */
Vector3f PathTracingMaterial::refractionSample(const Direction& wi, const Direction& normal, const float ni) const
{
	float cos_theta_i = glm::dot(-wi, normal);
	float eta_i = 1.0f;			// Refractive index of air
	float eta_t = this->ni;		// Refractive Index of material

	Direction n;
	/* Determine whether the light is entering from the outside or emitting from the inside */
	if (cos_theta_i < 0)
	{
		cos_theta_i = -cos_theta_i;
		n = -normal;
		std::swap(eta_i, eta_t);
	}
	else
	{
		n = normal;
	}

	float eta = eta_i / eta_t;
	float sin_theta_i = std::sqrt(1.0f - cos_theta_i * cos_theta_i);
	float sin_theta_t = eta * sin_theta_i;

	float R0 = pow((eta_i - eta_t) / (eta_i + eta_t), 2.0f);
	float fresnel = R0 + (1.0f - R0) * pow(1.0f - cos_theta_i, 5.0f);

	/* Check for total internal reflection */
	if (sin_theta_t >= 1.0f)
	{
		return reflect(wi, n);
	}

	float cos_theta_t = std::sqrt(1.0f - sin_theta_t * sin_theta_t);

	/* Calculating reflected light */
	Vector3f refracted = eta * wi + (eta * cos_theta_i - cos_theta_t) * n;

	return refracted;
}

float PathTracingMaterial::pdf(const Vector3f& wi, const Vector3f& wo, const Vector3f& normal) const
{
	if (this->type == MaterialType::Glossy)
	{
		return 1.0f / (2.0f * pi) * (1.0f / this->ns) + 1.0f * (1.0f - 1.0f / this->ns);
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
		Vector3f shader_color;
		if (this->diffuse_texture != -1)
		{
			shader_color = color;
		}
		else
		{
			shader_color = this->kd;
		}
		return std::max(glm::dot(normal, wo), 0.0f) * shader_color / glm::pi<float>();
	}
	else if (this->type == MaterialType::Glossy)
	{
		Vector3f shader_color;
		if (this->diffuse_texture != -1)
		{
			shader_color = color;
		}
		else
		{
			shader_color = this->kd;
		}

		Vector3f diffuse = std::max(glm::dot(normal, wo), 0.0f) * shader_color / glm::pi<float>();

		Vector3f half_direction = glm::normalize(wo + wi);
		Vector3f specular = std::pow(std::max(glm::dot(normal, half_direction), 0.0f), this->ns) * this->ks;

		return diffuse + specular;
	}
	else
	{
		return Vector3f{0.0f};
	}
}

float PathTracingMaterial::fresnel(const Vector3f& wi, const Vector3f& normal, const float& ni) const
{
	float cos_i = glm::clamp(glm::dot(wi, normal), -1.0f, 1.0f);
	float eta_i = 1.0f, eta_t = ni;

	if (cos_i < 0)
	{
		cos_i = -cos_i;
	}
	else
	{
		std::swap(eta_i, eta_t);
	}

	float sin_t = (eta_i / eta_t) * sqrtf(std::max(0.0f, 1.0f - cos_i * cos_i));

	if (sin_t >= 1.0f)
	{
		return 1.0f;
	}

	float cos_t = sqrtf(std::max(0.0f, 1.0f - sin_t * sin_t));
	float Rs = ((eta_t * cos_i) - (eta_i * cos_t)) / ((eta_t * cos_i) + (eta_i * cos_t));
	float Rp = ((eta_i * cos_i) - (eta_t * cos_t)) / ((eta_i * cos_i) + (eta_t * cos_t));

	return (Rs * Rs + Rp * Rp) * 0.5f;
}
