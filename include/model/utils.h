#pragma once

#include <chrono>
#include <iostream>
#include <map>
#include <random>
#include <vector>

#include <glm/ext/scalar_constants.hpp>
#include <glm/glm.hpp>

typedef glm::ivec2 Vector2i;
typedef glm::vec2 Vector2f;
typedef glm::dvec2 Vector2d;

typedef glm::ivec3 Vector3i;
typedef glm::vec3 Vector3f;
typedef glm::dvec3 Vector3d;

typedef glm::ivec4 Vector4i;
typedef glm::vec4 Vector4f;
typedef glm::dvec4 Vector4d;

typedef glm::mat4x4 Matrix4f;
typedef glm::mat3x3 Matrix3f;

typedef Vector3f Point;
typedef Vector3f Direction;
typedef Vector3f Coordinate3D;
typedef Vector2f Coordinate2D;

constexpr float pi = glm::pi<float>();

/**
 * @brief Clamps a value within a specified range.
 * @param[in] min The lower bound of the range.
 * @param[in] max The upper bound of the range.
 * @param[in] n The value to be clamped.
 * @return The clamped value within the range [min, max].
 */
float clamp(const float& min, const float& max, const float& n);

/**
 * @brief Generates a random floating-point number within a specified range.
 * @param[in] min The lower bound of the range.
 * @param[in] max The upper bound of the range.
 * @return A random number between min and max.
 */
float getRandomNumber(const float min, const float max);

/**
 * @brief Outputs the progress of a task.
 * @param[in] progress The progress value (typically between 0.0 and 1.0).
 */
void outputProgress(float progress);

/**
 * @brief Outputs the time duration of a task.
 * @param[in] name The name of the task.
 * @param[in] duration The time duration of the task.
 */
void outputTimeUse(std::string name, std::chrono::system_clock::duration duration);

/**
 * @brief Outputs the fps of a task.
 * @param[in] duration The time duration of the task.
 */
void outputFrameRate(std::chrono::system_clock::duration duration);
