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
	 * @brief Gets the horizontal texture coordinate (read-only).
	 *
	 * @return The U coordinate of the texture mapping.
	 */
	float u() const;

	/**
	 * @brief Gets a modifiable reference to the horizontal texture coordinate.
	 *
	 * @return A reference to the U coordinate of the texture mapping.
	 */
	float& u();

	/**
	 * @brief Gets the vertical texture coordinate (read-only).
	 *
	 * @return The V coordinate of the texture mapping.
	 */
	float v() const;

	/**
	 * @brief Gets a modifiable reference to the vertical texture coordinate.
	 *
	 * @return A reference to the V coordinate of the texture mapping.
	 */
	float& v();

	/**
	 * @brief Gets the X coordinate of the vertex in 3D space (read-only).
	 *
	 * @return The X position of the vertex.
	 */
	float x() const;

	/**
	 * @brief Gets a modifiable reference to the X coordinate of the vertex.
	 *
	 * @return A reference to the X position of the vertex.
	 */
	float& x();

	/**
	 * @brief Gets the Y coordinate of the vertex in 3D space (read-only).
	 *
	 * @return The Y position of the vertex.
	 */
	float y() const;

	/**
	 * @brief Gets a modifiable reference to the Y coordinate of the vertex.
	 *
	 * @return A reference to the Y position of the vertex.
	 */
	float& y();

	/**
	 * @brief Gets the Z coordinate of the vertex in 3D space (read-only).
	 *
	 * @return The Z position of the vertex.
	 */
	float z() const;

	/**
	 * @brief Gets a modifiable reference to the Z coordinate of the vertex.
	 *
	 * @return A reference to the Z position of the vertex.
	 */
	float& z();

	/* The position of the vertex in 3D space. */
	Coordinate3D position;

	/* The normal vector at the vertex, used for shading calculations. */
	Direction normal;

	/* The 2D texture coordinates associated with the vertex.*/
	Coordinate2D texture;

	Vector4f color;
};
