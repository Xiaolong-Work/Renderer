#pragma once

#include <utils.h>

/**
 * @class Vertex
 * @brief A class representing a 3D vertex.
 */
class Vertex
{
public:
	/**
	 * @brief Gets the horizontal texture coordinate.
	 *
	 * @return The U coordinate of the texture mapping.
	 */
	float u() const;

	/**
	 * @brief Gets the vertical texture coordinate.
	 *
	 * @return The V coordinate of the texture mapping.
	 */
	float v() const;

	/**
	 * @brief Gets the X coordinate of the vertex in 3D space.
	 *
	 * @return The X position of the vertex.
	 */
	float x() const;

	/**
	 * @brief Gets the Y coordinate of the vertex in 3D space.
	 *
	 * @return The Y position of the vertex.
	 */
	float y() const;

	/**
	 * @brief Gets the Z coordinate of the vertex in 3D space.
	 *
	 * @return The Z position of the vertex.
	 */
	float z() const;

	/**
	 * @brief Converts the vertex position to homogeneous coordinates.
	 *
	 * @return A 4D vector representing the vertex in homogeneous coordinates.
	 */
	Vector4f getHomogeneous() const;

	/* The position of the vertex in 3D space. */
	Point position;

	/* The normal vector at the vertex. */
	Direction normal;

	/* The 2D texture coordinates associated with the vertex. */
	Coordinate2D texture;
};
