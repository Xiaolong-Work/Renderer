#pragma once

#include <utils.h>

/**
 * @enum CameraType
 * @brief Defines the type of camera projection.
 */
enum class CameraType
{
	Perspective, /* Objects appear smaller as they get farther from the camera. */
	Orthographic /* Objects remain the same size regardless of depth. */
};

/**
 * @class Camera
 * @brief Represents a camera used for rendering a scene.
 */
class Camera
{
public:
	/* The name of the camera */
	std::string name;

	/* The width of the camera screen. */
	unsigned int width;

	/* The height of the camera screen. */
	unsigned int height;

	/* Field of view (FOV) in degrees, applicable for perspective projection. */
	float fov;

	float fovy;

	float aspect_ratio;

	float near_plane;

	float far_plane;

	/* The position of the camera in world space. */
	Coordinate3D position;

	/* The viewing direction of the camera. */
	Direction look;

	/* The up vector of the camera, defining its orientation. */
	Direction up;

	/* The type of projection used by the camera (Perspective or Orthographic). */
	CameraType type;
};