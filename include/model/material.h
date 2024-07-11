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
 * @class Material
 * @brief A class representing a material in a rendering system.
 */
class Material
{
public:
	/* The name of the material. */
	std::string name;

	/* Ambient reflectance component (Ka). */
	Vector3f Ka;

	/* Diffuse reflectance component (Kd). */
	Vector3f Kd;

	/* Specular reflectance component (Ks). */
	Vector3f Ks;

	/* Transmittance component (Tr). Default is white (fully transparent). */
	Vector3f Tr{1.0f, 1.0f, 1.0f};

	/* Shininess coefficient (glossiness factor). */
	float Ns;

	/* Refractive index of the material (Ni). */
	float Ni;

	/* Texture file path for diffuse mapping. */
	std::string map_Kd;

	/* The type of the material (diffuse, specular, etc.). */
	MaterialType type;

	/* Flag indicating whether a texture is loaded for this material. */
	bool texture_load_flag{false};

	/* The texture associated with the material. */
	Texture texture;
};