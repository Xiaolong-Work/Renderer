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

	/* The type of the material */
	MaterialType type;
};

class PBRMaterial
{
};
