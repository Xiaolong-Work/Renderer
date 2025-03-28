#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <math.h>
#include <optional>

#include <model.h>
#include <shader.h>
#include <triangle.h>
#include <utils.h>

class Rasterizer
{
public:
	Rasterizer() = default;
	Rasterizer(int width, int height);

	/* Set the pixel */
	void setPixel(Vector2i point, Vector3f color);

	/* Display the wireframe model */
	void drawLine(Vector4f begin, Vector4f end);

	/* Display the wireframe model */
	void drawWireframe(Model model);

	/* Converts the vertex position to homogeneous coordinates.*/
	Vector4f getHomogeneous(const Point& point);

	/* Determine if a point is inside a triangle */
	bool isInsideTriangle(int x, int y, const std::array<Vector4f, 3>& position);

	/* Calculate the center of gravity coordinates */
	Vector3f get2DBarycentric(float x, float y, const std::array<Vector4f, 3>& position);

	/* Draw a shaded triangle */
	void drawShaderTriangle(const std::array<Vector4f, 3>& position,
							const std::array<Direction, 3>& normal,
							const std::array<Coordinate2D, 3>& texture_coordinate,
							const std::array<Vector3f, 3>& viewspace_positions,
							Texture texture);

	/* Display the shaded triangle model */
	void drawShaderTriangleframe(Model model);

	/* Clear the buffer */
	void clear();

	/* Set the matrix of model transformation, view transformation and projection transformation */
	void setModel(Matrix4f model);
	void setView(Matrix4f view);
	void setProjection(Matrix4f projection);

	/* Model transformation matrix */
	Matrix4f model;

	/* View transformation matrix */
	Matrix4f view;

	/* Perspective transformation matrix */
	Matrix4f projection;

	/* Screen length and width */
	int width, height;

	/* Depth buffer */
	std::vector<float> depth_buffer;

	/* Screen buffer */
	std::vector<Vector4f> screen_buffer;

	/* shader */
	std::function<Vector3f(Shader)> shader;
};
