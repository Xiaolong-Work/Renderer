#include <transform.h>

Matrix4f getModelMatrix(Vector3f axis, float angle, Vector3f move)
{
	/* Rodringue rotation formula */
	float cos = std::cos(angle / 180.0 * PI);
	float sin = std::sin(angle / 180.0 * PI);
	Matrix3f I = Matrix3f(1.0f);
	Matrix3f rotate, temp;
	temp = {0, -axis[2], axis[1], axis[2], 0, -axis[0], -axis[1], axis[0], 0};
	rotate = cos * I + (1 - cos) * glm::outerProduct(axis, axis) + sin * temp;

	Matrix4f rotation = Matrix4f(1.0f);
	rotation[0] = glm::vec4(rotate[0], 0.0f);
	rotation[1] = glm::vec4(rotate[1], 0.0f);
	rotation[2] = glm::vec4(rotate[2], 0.0f);

	/* Generate a rotation matrix from translation and rotation */
	Matrix4f translate;
	translate = {1, 0, 0, move.x, 0, 1, 0, move.y, 0, 0, 1, move.z, 0, 0, 0, 1};

	/* Scaling Matrix */
	Matrix4f scale;
	scale = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
	Matrix4f model = translate * rotation * scale;
	return model;
}

Matrix4f getModelMatrix(Vector3f axis, float angle)
{
	Vector3f move{0, 0, 0};
	return getModelMatrix(axis, angle, move);
}

Matrix4f getModelMatrix(float angle)
{
	Vector3f axis{0, 1, 0};
	Vector3f move{0, 0, 0};
	return getModelMatrix(axis, angle, move);
}

Matrix4f getViewMatrix(Vector3f position, Vector3f look_towards, Vector3f up_direction)
{
	/* Calculate the rotation matrix */
	Matrix4f rotate;
	Vector3f temp = glm::cross(look_towards, up_direction);
	rotate = {temp.x,
			  temp.y,
			  temp.z,
			  0,
			  up_direction.x,
			  up_direction.y,
			  up_direction.z,
			  0,
			  -look_towards.x,
			  -look_towards.y,
			  -look_towards.z,
			  0,
			  0,
			  0,
			  0,
			  1};

	/* Calculate the translation matrix */
	Matrix4f translate;
	translate = {1, 0, 0, -position[0], 0, 1, 0, -position[1], 0, 0, 1, -position[2], 0, 0, 0, 1};

	/* Calculate the view change matrix */
	Matrix4f view;
	view = rotate * translate;
	return view;
}

Matrix4f getViewMatrix(Vector3f position)
{
	Vector3f look_towards{0, 0, -1};
	Vector3f up_direction{0, 1, 0};
	return getViewMatrix(position, look_towards, up_direction);
}

Matrix4f getProjectionMatrix(float fov, float aspect_ratio, float z_near, float z_far)
{
	/* Orthographic projection */
	// double top = std::tan(fov / 180.0 * PI / 2.0) * z_near;
	double top = -std::tan(fov / 180.0 * PI / 2.0) * z_near;
	double bottom = -top;
	double right = aspect_ratio * top;
	double left = -right;

	/* Translation process */
	Matrix4f move = {
		1, 0, 0, -(right + left) / 2.0, 0, 1, 0, -(top + bottom) / 2.0, 0, 0, 1, -(z_near + z_far) / 2.0, 0, 0, 0, 1};

	/* Scaling process */
	Matrix4f resize = {
		2.0 / (right - left), 0, 0, 0, 0, 2.0 / (top - bottom), 0, 0, 0, 0, 2.0 / (z_near - z_far), 0, 0, 0, 0, 1};

	/* Orthographic projection */
	Matrix4f orthographics;
	orthographics = resize * move;

	/* Compression transformation, transforming the frustum into a cuboid */
	Matrix4f perspective;
	perspective = {z_near, 0, 0, 0, 0, z_near, 0, 0, 0, 0, z_near + z_far, -z_far * z_near, 0, 0, 1, 0};

	/* Perspective projection matrix */
	Matrix4f projection;
	projection = orthographics * perspective;
	return projection;
}
