#pragma once

#include <utils.h>

#define PI pi

/* Get the model transformation matrix */ /* model */
Matrix4f getModelMatrix(Vector3f axis, float angle, Vector3f move);
Matrix4f getModelMatrix(Vector3f axis, float angle);
Matrix4f getModelMatrix(float angle);

/* Get the view transformation matrix */ /* view */
Matrix4f getViewMatrix(Vector3f position, Vector3f look_towards, Vector3f up_direction);
Matrix4f getViewMatrix(Vector3f position);

/* Get the perspective projection matrix */ /* project */
Matrix4f getProjectionMatrix(float fov, float aspect_ratio, float z_near, float z_far);
