#pragma once

#include <cmath>
#include <iostream>

#include <glm/ext/scalar_constants.hpp>

#include <data_loader.h>
#include <texture.h>
#include <utils.h>

/**
 * @class PathTracingMaterial
 * @brief Material Methods in Path Tracing class
 */
class PathTracingMaterial : public Material
{
public:
	PathTracingMaterial() = default;
	PathTracingMaterial(Material& material);

	/**
	 * @brief Samples a direction for a given incident direction and surface normal.
	 *
	 * @param[in] wi The incident direction.
	 * @param[in] normal The surface normal.
	 * @return A sampled direction for light reflection or transmission.
	 */
	Vector3f sample(const Direction& wi, const Direction& normal) const;

	/**
	 * @brief Samples a direction for glossy reflection.
	 *
	 * @param[in] wi The incident direction.
	 * @param[in] normal The surface normal.
	 * @return The sampled reflection direction for a glossy surface.
	 */
	Vector3f glossySample(const Direction& wi, const Direction& normal) const;

	/**
	 * @brief Samples a direction for diffuse reflection.
	 *
	 * @param[in] normal The surface normal.
	 * @return A randomly sampled diffuse reflection direction.
	 */
	Vector3f diffuseSample(const Direction& normal) const;

	/**
	 * @brief Samples a direction for refraction.
	 *
	 * @param[in] wi The incident direction.
	 * @param[in] normal The surface normal.
	 * @param[in] ni The refractive index of the material.
	 * @return The sampled refraction direction.
	 */
	Vector3f refractionSample(const Direction& wi, const Direction& normal, const float ni) const;

	/**
	 * @brief Samples a direction for specular reflection.
	 *
	 * @param[in] wi The incident direction.
	 * @param[in] normal The surface normal.
	 * @return The perfect mirror reflection direction.
	 */
	Vector3f reflectSample(const Direction& wi, const Direction& normal) const;

	/**
	 * @brief Computes the probability density function (PDF) for a sampled direction.
	 *
	 * @param[in] wi The incident direction.
	 * @param[in] wo The outgoing direction.
	 * @param[in] normal The surface normal.
	 * @return The probability density of sampling wo given wi.
	 */
	float pdf(const Vector3f& wi, const Vector3f& wo, const Vector3f& normal) const;

	/**
	 * @brief Evaluates the material's shading model.
	 *
	 * @param[in] wi The incident direction.
	 * @param[in] wo The outgoing direction.
	 * @param[in] normal The surface normal.
	 * @param[in] color The base color of the material.
	 * @return The computed shading color.
	 */
	Vector3f evaluate(const Vector3f& wi, const Vector3f& wo, const Vector3f& normal, const Vector3f& color) const;

	/**
	 * @brief Computes the Fresnel term for reflectance.
	 *
	 * @param[in] wi The incident direction.
	 * @param[in] normal The surface normal.
	 * @param[in] ni The refractive index of the material.
	 * @return The Fresnel reflection coefficient.
	 */
	float fresnel(const Vector3f& wi, const Vector3f& normal, const float& ni) const;
};
