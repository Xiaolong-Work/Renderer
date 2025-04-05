#pragma once

#include <utils.h>

/**
 * @class BoundingBox
 * @brief A class representing a 3D AABB bounding box.
 */
class BoundingBox
{
public:
	/**
	 * @brief Default constructor for an empty bounding box.
	 */
	BoundingBox();

	/**
	 * @brief Constructor that initializes a bounding box with a single point.
	 *
	 * @param[in] point : A point to initialize the bounding box.
	 */
	BoundingBox(const Point& point);

	/**
	 * @brief Constructor that initializes a bounding box with a minimum and maximum point.
	 *
	 * @param[in] min : The minimum corner point of the bounding box.
	 * @param[in] max : The maximum corner point of the bounding box.
	 */
	BoundingBox(const Point& min, const Point& max);

	/**
	 * @brief Combines the current bounding box with another bounding box.
	 *
	 * @param[in] other : The bounding box to combine with the current one.
	 */
	void unionBox(const BoundingBox& other);

	/**
	 * @brief Expands the bounding box to include a point.
	 *
	 * @param[in] other : The point to include within the bounding box.
	 */
	void unionPoint(const Point& other);

	/**
	 * @brief Gets the minimum point of the bounding box.
	 *
	 * @return The minimum point of the bounding box.
	 */
	Point getMin() const;

	/**
	 * @brief Gets the maximum point of the bounding box.
	 *
	 * @return The maximum point of the bounding box.
	 */
	Point getMax() const;

	/**
	 * @brief Computes the length of the bounding box along the X-axis.
	 *
	 * @return The length along the X-axis.
	 */
	float getLengthX() const;

	/**
	 * @brief Computes the length of the bounding box along the Y-axis.
	 *
	 * @return The length along the Y-axis.
	 */
	float getLengthY() const;

	/**
	 * @brief Computes the length of the bounding box along the Z-axis.
	 *
	 * @return The length along the Z-axis.
	 */
	float getLengthZ() const;

	/**
	 * @brief Checks if the bounding box overlaps with another bounding box.
	 *
	 * @param[in] other : The bounding box to check for overlap.
	 * @return True if the bounding boxes overlap, false otherwise.
	 */
	bool overlaps(const BoundingBox& other) const;

	/**
	 * @brief Checks if the bounding box overlaps with a point.
	 *
	 * @param[in] other : The point to check for overlap.
	 * @return True if the point is inside the bounding box, false otherwise.
	 */
	bool overlaps(const Point& other) const;

	/* The minimum and maximum coordinates of the bounding box along each axis. */
	float x_min, x_max;
	float y_min, y_max;
	float z_min, z_max;
};
