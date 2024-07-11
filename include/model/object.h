#pragma once

#include <string>
#include <vector>

#include <material.h>
#include <triangle.h>

/**
 * @class Object
 * @brief Represents a geometric object in the scene.
 */
class Object
{
public:
	/* The name of the object. */
	std::string name;

	/* The triangle mesh representing the object's geometry. */
	std::vector<Triangle> mesh;

	/* The material of the object. */
	Material material;

	/* The bounding box of the object. */
	BoundingBox bounding_box{};

	/* The total surface area of the object. */
	float area = 0.0f;

	/* Flag indicating whether the object is a light source. */
	bool is_light{false};

	/* The emitted radiance of the object if it is a light source. */
	Vector3f radiance{0.0f, 0.0f, 0.0f};
};