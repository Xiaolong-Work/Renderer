#pragma once

#include <bvh.h>
#include <object.h>
#include <path_tracing_material.h>
#include <ray.h>

/**
 * @class PathTracingObject
 * @brief Represents a geometric object in the scene.
 */
class PathTracingObject : public Object
{
public:
	/**
	 * @brief Default constructor for PathTracingObject.
	 */
	PathTracingObject() = default;
	PathTracingObject(Object& object);

	/**
	 * @brief Initializes the BVH structure for the object.
	 */
	void initBVH();

	/**
	 * @brief Builds the BVH tree for this object.
	 *
	 * @param[in] index The index of the BVH node.
	 * @param[in,out] triangles The list of triangles belonging to this object.
	 * @param[in] bounding_box The bounding box enclosing the triangles.
	 */
	void buildBVH(const int index, std::vector<Triangle>& triangles, const BoundingBox& bounding_box);

	/**
	 * @brief Computes the intersection between a ray and this object.
	 *
	 * @param[in,out] ray The ray to be tested for intersection.
	 * @return The intersection result containing intersection details.
	 */
	IntersectResult intersect(Ray& ray) const;

	/**
	 * @brief Traverses the BVH structure to find ray intersections.
	 *
	 * @param[in] index The index of the BVH node to traverse.
	 * @param[in,out] ray The ray to test for intersection.
	 * @return The intersection result.
	 */
	IntersectResult traverse(const int index, Ray& ray) const;

	/**
	 * @brief Samples a point on the surface of a light-emitting object.
	 *
	 * @param[out] result The intersection result storing the sampled point.
	 * @param[out] pdf The probability density function (PDF) value of the sample.
	 */
	void sample(IntersectResult& result, float& pdf);

	/**
	 * @brief Traverses the BVH structure to sample a point on the object.
	 *
	 * @param[in] index The BVH node index.
	 * @param[in] p A random parameter for sampling.
	 * @param[out] result The intersection result storing the sampled point.
	 * @param[out] pdf The probability density function (PDF) value of the sample.
	 */
	void traverse(const int index, float p, IntersectResult& result, float& pdf);

	/* The BVH tree of the object. */
	std::vector<BVH> bvh;

	/* The material of the object. */
	PathTracingMaterial material;
};