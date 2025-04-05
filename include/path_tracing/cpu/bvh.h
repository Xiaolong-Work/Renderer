#pragma once

#include <algorithm>

#include <bounding_box.h>
#include <data_loader.h>
#include <material.h>
#include <ray.h>
#include <triangle.h>
#include <utils.h>

/**
 * @class BVH
 * @brief Bounding Volume Hierarchy (BVH) node structure for accelerating ray tracing.
 */
class BVH
{
public:
	/* The bounding box of this BVH node. */
	BoundingBox bounding_box{};

	/* The total surface area of the geometry stored in this node. */
	float area = 0.0f;

	/* Index of the left child node (-1 if none). */
	int left = -1;

	/* Index of the right child node (-1 if none). */
	int right = -1;

	/* Flag indicating whether this node is a leaf node. */
	bool leaf_node_flag{false};

	/* The triangle stored in this leaf node (valid only if it is a leaf). */
	Triangle triangle{};
};

/**
 * @class SceneBVH
 * @brief A BVH node specifically for scenes.
 */
class SceneBVH : public BVH
{
public:
	/* Index of the object stored in this leaf node. */
	int object_index;
};

/**
 * @class IntersectResult
 * @brief Stores the results of a ray-object intersection test.
 */
class IntersectResult
{
public:
	/* Flag indicating whether an intersection occurred. */
	bool is_intersect{false};

	/* The intersecting ray. */
	Ray ray;

	/* The distance along the ray at which the intersection occurred. */
	float t = std::numeric_limits<float>::infinity();

	/* The intersection point or sampled point. */
	Point point;

	/* The index of the intersected object. */
	int object_index;

	/* The interpolated color at the intersection point. */
	Vector3f color{0.0f, 0.0f, 0.0f};

	/* The interpolated normal at the intersection point. */
	Direction normal;

	Coordinate2D uv;
};
