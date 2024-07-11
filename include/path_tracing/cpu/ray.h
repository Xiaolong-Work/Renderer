#pragma once

#include <bounding_box.h>
#include <triangle.h>
#include <utils.h>

/**
 * @class Ray
 * @brief A class representing a ray in 3D space.
 */
class Ray
{
public:
	/**
	 * @brief Default constructor for an uninitialized ray..
	 */
	Ray() = default;

	/**
	 * @brief Constructs a ray with a specified origin and direction.
	 *
	 * @param[in] point The origin of the ray.
	 * @param[in] direction The direction of the ray.
	 */
	Ray(const Point& point, const Direction& direction);

	/**
	 * @brief Computes the position of a point along the ray at a given distance.
	 *
	 * @param[in] t The distance along the ray.
	 * @return The computed point along the ray.
	 */
	Point spread(const float t) const;

	/**
	 * @brief Checks if the ray intersects with a given triangle.
	 *
	 * @param[in] triangle The triangle to test for intersection.
	 * @param[out] result Array to store intersection details (e.g., barycentric coordinates, distance).
	 * @return True if the ray intersects the triangle, false otherwise.
	 */
	bool intersectTriangle(const Triangle& triangle, float result[4]) const;

	/**
	 * @brief Checks if the ray intersects an axis-aligned bounding box (AABB).
	 *
	 * @param[in] box The bounding box to test for intersection.
	 * @return True if the ray intersects the bounding box, false otherwise.
	 */
	bool intersectBoundingBox(const BoundingBox& box) const;

	/* The origin point of the ray. */
	Point origin;

	/* The normalized direction of the ray. */
	Direction direction;

	/* The propagation distance of the ray.Initialized to infinity by default. */
	float t = std::numeric_limits<float>::infinity();
};
