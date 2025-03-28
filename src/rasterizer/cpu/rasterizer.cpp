#include <rasterizer.h>

Vector4f Rasterizer::getHomogeneous(const Point& point)
{
	return Vector4f{point, 1.0f};
}

Rasterizer::Rasterizer(int width, int height)
{
	this->width = width;
	this->height = height;
	this->screen_buffer.resize(width * height);
	this->depth_buffer.resize(width * height);
}

void Rasterizer::setPixel(Vector2i point, Vector3f color)
{
	if (point.x < 0 || point.x >= width || point.y < 0 || point.y >= height)
		return;

	int index = (height - 1 - point.y) * width + point.x;
	screen_buffer[index] = Vector4f(color, 1.0f);
}

void Rasterizer::drawLine(Vector4f begin, Vector4f end)
{
	auto x0 = begin.x;
	auto y0 = begin.y;
	auto x1 = end.x;
	auto y1 = end.y;

	/* Determine whether the independent variable is x or y based on the slope */
	bool steep = false;
	if (std::abs(x0 - x1) < std::abs(y0 - y1))
	{
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}

	/* Make the line always drawn from left to right */
	if (x0 > x1)
	{
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	/* Perform line operation */
	Vector3f color{1.0f};
	Vector2i point;
	for (int x = x0; x <= x1; x++)
	{
		float t = (x - x0) / (float)(x1 - x0);
		int y = y0 * (1.0 - t) + y1 * t;
		if (steep)
		{
			point = {y, x};
			setPixel(point, color);
		}
		else
		{
			point = {x, y};
			setPixel(point, color);
		}
	}
}

void Rasterizer::drawWireframe(Model model)
{
	Matrix4f mvp = this->projection * this->view * this->model;

#pragma omp parallel for
	for (auto& trangle : model.faces)
	{
		Vector4f a = getHomogeneous(trangle.vertex1.position);
		Vector4f b = getHomogeneous(trangle.vertex2.position);
		Vector4f c = getHomogeneous(trangle.vertex3.position);

		a = mvp * a;
		b = mvp * b;
		c = mvp * c;

		a /= a.w;
		b /= b.w;
		c /= c.w;

		a.x = 0.5 * width * (a.x + 1.0);
		a.y = 0.5 * height * (a.y + 1.0);
		b.x = 0.5 * width * (b.x + 1.0);
		b.y = 0.5 * height * (b.y + 1.0);
		c.x = 0.5 * width * (c.x + 1.0);
		c.y = 0.5 * height * (c.y + 1.0);

		drawLine(c, a);
		drawLine(c, b);
		drawLine(b, a);
	}
}

bool Rasterizer::isInsideTriangle(int x, int y, const std::array<Vector4f, 3>& position)
{
	Vector3f edge, temp;
	auto& v0 = position[0];
	auto& v1 = position[1];
	auto& v2 = position[2];

	/* v[0] -> v[1] (V0V1) x v[0] -> Q (V0Q) */
	edge = {v1.x - v0.x, v1.y - v0.y, 0};
	temp = {x - v0.x, y - v0.y, 0};
	auto flag1 = glm::cross(edge, temp).z;

	/* v[1] -> v[2] (V1V2) x v[1] -> Q (V1Q) */
	edge = {v2.x - v1.x, v2.y - v1.y, 0};
	temp = {x - v1.x, y - v1.y, 0};
	auto flag2 = glm::cross(edge, temp).z;

	/* v[2] -> v[0] (V2V0) x v[2] -> Q (V2Q) */
	edge = {v0.x - v2.x, v0.y - v2.y, 0};
	temp = {x - v2.x, y - v2.y, 0};
	auto flag3 = glm::cross(edge, temp).z;

	if (flag1 >= 0 && flag2 >= 0 && flag3 >= 0)
	{
		return true;
	}
	else if (flag1 <= 0 && flag2 <= 0 && flag3 <= 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

Vector3f Rasterizer::get2DBarycentric(float x, float y, const std::array<Vector4f, 3>& position)
{
	auto& v0 = position[0];
	auto& v1 = position[1];
	auto& v2 = position[2];

	float c1 = (x * (v1.y - v2.y) + (v2.x - v1.x) * y + v1.x * v2.y - v2.x * v1.y) /
			   (v0.x * (v1.y - v2.y) + (v2.x - v1.x) * v0.y + v1.x * v2.y - v2.x * v1.y);
	float c2 = (x * (v2.y - v0.y) + (v0.x - v2.x) * y + v2.x * v0.y - v0.x * v2.y) /
			   (v1.x * (v2.y - v0.y) + (v0.x - v2.x) * v1.y + v2.x * v0.y - v0.x * v2.y);

	return Vector3f{c1, c2, 1.0f - c1 - c2};
}

void Rasterizer::drawShaderTriangle(const std::array<Vector4f, 3>& position,
									const std::array<Direction, 3>& normal,
									const std::array<Coordinate2D, 3>& texture_coordinate,
									const std::array<Vector3f, 3>& viewspace_positions,
									Texture texture)
{
	auto& v0 = position[0];
	auto& v1 = position[1];
	auto& v2 = position[2];

	/* Calculate the bounding box of the triangle */
	auto x_max = std::max(v0.x, std::max(v1.x, v2.x));
	auto x_min = std::ceil(std::min(v0.x, std::min(v1.x, v2.x)));
	auto y_max = std::ceil(std::max(v0.y, std::max(v1.y, v2.y)));
	auto y_min = std::min(v0.y, std::min(v1.y, v2.y));

	/* For each pixel in the bounding box, determine whether it is inside the triangle and rasterize the triangle */
#pragma omp parallel for
	for (int y = std::min((int)y_max, height - 1); y >= y_min && y >= 0; y--)
	{
		for (int x = std::max(0, (int)x_min); x <= x_max && x <= width; x++)
		{
			/* Check if it is inside the triangle */
			if (isInsideTriangle(x, y, position))
			{
				/* Get the center of gravity coordinates */
				float alpha = get2DBarycentric(x, y, position).x;
				float beta = get2DBarycentric(x, y, position).y;
				float gamma = get2DBarycentric(x, y, position).z;

				/* Perform interpolation on depth */
				float w_reciprocal = 1.0 / (alpha / v0.w + beta / v1.w + gamma / v2.w);
				float z_interpolated = alpha * v0.z / v0.w + beta * v1.z / v1.w + gamma * v2.z / v2.w;
				z_interpolated *= w_reciprocal;

				/* Depth buffer */
				if (z_interpolated < depth_buffer[(height - 1 - y) * width + x])
				{
					depth_buffer[(height - 1 - y) * width + x] = z_interpolated;

					/* Calculate the interpolation value of each data */
					auto interpolated_normal = alpha * normal[0] + beta * normal[1] + gamma * normal[2];
					auto interpolated_texture_coordinate =
						alpha * texture_coordinate[0] + beta * texture_coordinate[1] + gamma * texture_coordinate[2];
					auto interpolated_shading_point =
						alpha * viewspace_positions[0] + beta * viewspace_positions[1] + gamma * viewspace_positions[2];

					/* Load shading data */
					Shader shader{};
					shader.normal = glm::normalize(interpolated_normal);
					shader.texture_coordinate = interpolated_texture_coordinate;
					shader.shading_point = interpolated_shading_point;
					shader.texture = &texture;

					/* Shader calculation shading */
					auto pixel_color = this->shader(shader);
					Vector2i point{x, y};
					setPixel(point, pixel_color);
				}
			}
		}
	}
}

void Rasterizer::drawShaderTriangleframe(Model model)
{
	Matrix4f mvp = this->projection * this->view * this->model;
	Matrix4f mv = this->view * this->model;

	std::vector<std::array<Vector4f, 3>> positions;
	std::vector<std::array<Direction, 3>> normals;
	std::vector<std::array<Coordinate2D, 3>> texture_coordinates;
	std::vector<std::array<Point, 3>> viewspace_positions;

	size_t size = model.faces.size();
	positions.resize(size);
	normals.resize(size);
	texture_coordinates.resize(size);
	viewspace_positions.resize(size);

#pragma omp parallel for
	for (size_t i = 0; i < size; i++)
	{
		auto& triangle = model.faces[i];

		auto& a = triangle.vertex1;
		auto& b = triangle.vertex2;
		auto& c = triangle.vertex3;

		/* Transform shading point */
		auto trans_shader_point = [mv, this](const Point& point) { return Point(mv * this->getHomogeneous(point)); };
		viewspace_positions[i] = {
			trans_shader_point(a.position), trans_shader_point(b.position), trans_shader_point(c.position)};

		/* Transform the normal vector */
		Matrix4f inverse_transform = glm::transpose(glm::inverse(mv));
		auto trans_normal = [inverse_transform](const Direction& normal) {
			return Vector3f(inverse_transform * Vector4f(normal, 0.0f));
		};
		normals[i] = {trans_normal(a.normal), trans_normal(b.normal), trans_normal(c.normal)};

		/* Transform the position */
		auto trans_position = [mvp, this](const Point& point) {
			Vector4f result = mvp * this->getHomogeneous(point);
			result.x = 0.5 * this->width * (result.x / result.w + 1.0);
			result.y = 0.5 * this->height * (result.y / result.w + 1.0);
			return result;
		};
		positions[i] = {trans_position(a.position), trans_position(b.position), trans_position(c.position)};

		texture_coordinates[i] = {a.texture, b.texture, c.texture};
	}

	for (size_t i = 0; i < size; i++)
	{
		drawShaderTriangle(positions[i], normals[i], texture_coordinates[i], viewspace_positions[i], model.texture);
	}
}

void Rasterizer::clear()
{
	std::fill(screen_buffer.begin(), screen_buffer.end(), Vector4f{0, 0, 0, 0});
	std::fill(depth_buffer.begin(), depth_buffer.end(), std::numeric_limits<float>::infinity());
}

void Rasterizer::setModel(Matrix4f model)
{
	this->model = model;
}

void Rasterizer::setView(Matrix4f view)
{
	this->view = view;
}

void Rasterizer::setProjection(Matrix4f projection)
{
	this->projection = projection;
}


