#pragma once

#include <string>

#include <texture.h>
#include <utils.h>

/**
 * @enum MaterialType
 * @brief Defines different types of materials used in rendering.
 */
enum class MaterialType
{
	Diffuse,	/* Diffuse material (Lambertian reflection). */
	Specular,	/* Specular reflection (mirror-like reflection). */
	Refraction, /* Refractive material (light transmission through the surface). */
	Glossy		/* Glossy material (partial reflection and refraction). */
};

/**
 * @struct Material
 * @brief A struct representing a material in a rendering system.
 */
struct Material
{
	/* The name of the material. */
	std::string name;

	/* ========== Blinn-Phong ========== */
	/* Ambient reflectance component (ka). */
	Vector3f ka{0.0f};

	/* Diffuse reflectance component (kd). */
	Vector3f kd{0.0f};

	/* Specular reflectance component (ks). */
	Vector3f ks{0.0f};

	/* Transmittance component (tr). */
	Vector3f tr{0.0f};

	/* Shininess coefficient (glossiness factor). */
	float ns{0.0f};

	/* Refractive index of the material (ni). */
	float ni{0.0f};

	/* The diffuse texture index of the object, -1 means no texture data */
	Index diffuse_texture{-1};

	/* The specular texture index of the object, -1 means no texture data */
	Index specular_texture{-1};

	/* The type of the material */
	MaterialType type;

	/* ========== PBR ========== */
	Vector4f albedo;
	float metallic;
	float roughness;

	Index albedo_texture{-1};

	/* ========== Double layer material ========== */
	Vector4f bottom_albedo;
	float bottom_metallic;
	float bottom_roughness;

	Index bottom_albedo_texture{-1};

	void transferToPBR()
	{
		switch (this->type)
		{
		case MaterialType::Diffuse:
			{
				this->roughness = 1.0;
				this->metallic = 0.0;
				this->albedo = Vector4f{kd, 1.0f};
				this->albedo_texture = this->diffuse_texture;
				break;
			}
		case MaterialType::Specular:
			{
				this->roughness = 0.0;
				this->metallic = 1.0;
				this->albedo = Vector4f{ks, 1.0f};
				this->albedo_texture = this->specular_texture;
				break;
			}
		case MaterialType::Refraction:
			{
				this->roughness = 0.0;
				this->metallic = 0.0;
				this->albedo = Vector4f{ks, 1.0f};
				this->albedo_texture = this->specular_texture;
				break;
			}
		case MaterialType::Glossy:
			{
				/* Top layer (mirror layer) */
				this->roughness = clamp(0.0, 1.0, std::sqrt(2.0 / (this->ns + 2.0)));
				this->metallic = 0.0;
				this->albedo = Vector4f{ks, 1.0f};
				this->albedo_texture = this->specular_texture;
				this->ni = 1.5;

				/* Base layer (diffuse reflection layer) */
				this->bottom_roughness = 1.0;
				this->bottom_metallic = 0.0;
				this->bottom_albedo = Vector4f{kd, 1.0f};
				this->bottom_albedo_texture = this->diffuse_texture;
				break;
			}
		default:
			{
				break;
			}
		}
		
		this->metallic = clamp(0.0, 1.0, length(ks) / (length(ks) + length(kd) + 0.0001));
		this->albedo_texture = this->diffuse_texture;
	}
};
