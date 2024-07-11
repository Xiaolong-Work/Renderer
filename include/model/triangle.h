#pragma once

#include <bounding_box.h>
#include <utils.h>
#include <vertex.h>

/**
 * @class Triangle
 * @brief A class representing a 3D triangle.
 */
class Triangle
{
public:
	/** Default constructor for a triangle. */
	Triangle() = default;

	/**
	 * @brief Constructs a triangle with three vertices.
	 *
	 * @param[in] vertex1 : The first vertex of the triangle.
	 * @param[in] vertex2 : The second vertex of the triangle.
	 * @param[in] vertex3 : The third vertex of the triangle.
	 */
	Triangle(const Vertex& vertex1, const Vertex& vertex2, const Vertex& vertex3);

	/**
	 * @brief Computes the bounding box of the triangle.
	 *
	 * @return The bounding box that contains the triangle.
	 */
	BoundingBox getBoundingBox() const;

	/**
	 * @brief Samples a point on the triangle and computes its PDF (Probability Density Function).
	 *
	 * @param[out] result : The sampled point on the triangle.
	 * @param[out] pdf : The PDF of the sampled point.
	 */
	void sample(Point& result, float& pdf) const;

	/* The three vertices of the triangle. */
	Vertex vertex1, vertex2, vertex3;

	/* The three edges of the triangle. */
	Direction edge1, edge2, edge3;

	/* The normal vector of the triangle. */
	Direction normal;

	/* The area of the triangle. */
	float area;

	/* The bounding box of the triangle. */
	BoundingBox bounding_box;
};