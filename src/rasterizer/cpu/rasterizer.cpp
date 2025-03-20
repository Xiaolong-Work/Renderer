#include <rasterizer.h>

Rasterizer::Rasterizer(int width, int height)
{
	this->width = width;
	this->height = height;
	screen_buffer.resize(width * height);
	depth_buffer.resize(width * height);
}

void Rasterizer::setPixel(Vector2i point, Vector3f color)
{
	if (point.x < 0 || point.x >= width || point.y < 0 || point.y >= height)
		return;

	int index = (height - 1 - point.y) * width + point.x;
	screen_buffer[index] = color;
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
	Vector3f color{255, 255, 255};
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
	for (Triangle trangle : model.faces)
	{
		Vector4f a = trangle.vertex1.getHomogeneous();
		Vector4f b = trangle.vertex2.getHomogeneous();
		Vector4f c = trangle.vertex3.getHomogeneous();

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

bool Rasterizer::isInsideTriangle(int x, int y, Triangle triangle)
{
	Vector3f edge, temp;
	Vertex v0 = triangle.vertex1;
	Vertex v1 = triangle.vertex2;
	Vertex v2 = triangle.vertex3;

	/* v[0] -> v[1] (V0V1) x v[0] -> Q (V0Q) */
	edge = {v1.x() - v0.x(), v1.y() - v0.y(), 0};
	temp = {x - v0.x(), y - v0.y(), 0};
	auto flag1 = glm::cross(edge, temp).z;

	/* v[1] -> v[2] (V1V2) x v[1] -> Q (V1Q) */
	edge = {v2.x() - v1.x(), v2.y() - v1.y(), 0};
	temp = {x - v1.x(), y - v1.y(), 0};
	auto flag2 = glm::cross(edge, temp).z;

	/* v[2] -> v[0] (V2V0) x v[2] -> Q (V2Q) */
	edge = {v0.x() - v2.x(), v0.y() - v2.y(), 0};
	temp = {x - v2.x(), y - v2.y(), 0};
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

std::tuple<float, float, float> Rasterizer::get2DBarycentric(float x, float y, Triangle triangle)
{
	Vertex v0 = triangle.vertex1;
	Vertex v1 = triangle.vertex2;
	Vertex v2 = triangle.vertex3;

	float c1 = (x * (v1.y() - v2.y()) + (v2.x() - v1.x()) * y + v1.x() * v2.y() - v2.x() * v1.y()) /
			   (v0.x() * (v1.y() - v2.y()) + (v2.x() - v1.x()) * v0.y() + v1.x() * v2.y() - v2.x() * v1.y());
	float c2 = (x * (v2.y() - v0.y()) + (v0.x() - v2.x()) * y + v2.x() * v0.y() - v0.x() * v2.y()) /
			   (v1.x() * (v2.y() - v0.y()) + (v0.x() - v2.x()) * v1.y() + v2.x() * v0.y() - v0.x() * v2.y());
	float c3 = (x * (v0.y() - v1.y()) + (v1.x() - v0.x()) * y + v0.x() * v1.y() - v1.x() * v0.y()) /
			   (v2.x() * (v0.y() - v1.y()) + (v1.x() - v0.x()) * v2.y() + v0.x() * v1.y() - v1.x() * v0.y());
	return {c1, c2, c3};
}

void Rasterizer::drawTriangle(Triangle triangle)
{
	Vertex v0 = triangle.vertex1;
	Vertex v1 = triangle.vertex2;
	Vertex v2 = triangle.vertex3;

	/* Calculate the bounding box of the triangle */
	auto x_max = std::max(v0.x(), std::max(v1.x(), v2.x()));
	auto x_min = std::ceil(std::min(v0.x(), std::min(v1.x(), v2.x())));
	auto y_max = std::ceil(std::max(v0.y(), std::max(v1.y(), v2.y())));
	auto y_min = std::min(v0.y(), std::min(v1.y(), v2.y()));

	/* For each pixel in the bounding box, determine whether it is inside the triangle and rasterize the triangle */
	for (int y = (int)y_max; y >= y_min; y--)
	{
		for (int x = (int)x_min; x <= x_max; x++)
		{
			/* Check if it is inside the triangle */
			if (isInsideTriangle(x, y, triangle))
			{
				/* Get the center of gravity coordinates */
				auto [alpha, beta, gamma] = get2DBarycentric(x, y, triangle);

				/* Perform interpolation on depth */
				float w_reciprocal = 1.0 / (alpha / v0.w() + beta / v1.w() + gamma / v2.w());
				float z_interpolated = alpha * v0.z() / v0.w() + beta * v1.z() / v1.w() + gamma * v2.z() / v2.w();
				z_interpolated *= w_reciprocal;

				/* Depth buffer */
				if (z_interpolated < depth_buffer[(height - 1 - y) * width + x])
				{
					depth_buffer[(height - 1 - y) * width + x] = z_interpolated;

					Vector2i point;
					point = {x, y};
					setPixel(point, v0.color);
				}
			}
		}
	}
}

void Rasterizer::drawTriangleframe(Model model)
{
	Matrix4f mvp = this->projection * this->view * this->model;
	for (Triangle trangle : model.faces)
	{
		Vector4f a = trangle.vertex1.getHomogeneous();
		Vector4f b = trangle.vertex2.getHomogeneous();
		Vector4f c = trangle.vertex3.getHomogeneous();

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

		Triangle temp = trangle;
		temp.vertex1.position = Vector3f(a);
		temp.vertex2.position = Vector3f(b);
		temp.vertex3.position = Vector3f(c);
		drawTriangle(temp);
	}
}

void Rasterizer::drawShaderTriangle(Triangle triangle, const std::array<Vector3f, 3>& view_pos, Texture texture)
{
	Vertex v0 = triangle.vertex1;
	Vertex v1 = triangle.vertex2;
	Vertex v2 = triangle.vertex3;

	/* Calculate the bounding box of the triangle */
	auto x_max = std::max(v0.x(), std::max(v1.x(), v2.x()));
	auto x_min = std::ceil(std::min(v0.x(), std::min(v1.x(), v2.x())));
	auto y_max = std::ceil(std::max(v0.y(), std::max(v1.y(), v2.y())));
	auto y_min = std::min(v0.y(), std::min(v1.y(), v2.y()));

	/* For each pixel in the bounding box, determine whether it is inside the triangle and rasterize the triangle */
	for (int y = std::min((int)y_max, height - 1); y >= y_min && y >= 0; y--)
	{
		for (int x = std::max(0, (int)x_min); x <= x_max && x <= width; x++)
		{
			/* Check if it is inside the triangle */
			if (isInsideTriangle(x, y, triangle))
			{
				/* Get the center of gravity coordinates */
				auto [alpha, beta, gamma] = get2DBarycentric(x, y, triangle);

				/* Perform interpolation on depth */
				float w_reciprocal = 1.0 / (alpha / v0.w() + beta / v1.w() + gamma / v2.w());
				float z_interpolated = alpha * v0.z() / v0.w() + beta * v1.z() / v1.w() + gamma * v2.z() / v2.w();
				z_interpolated *= w_reciprocal;

				/* Depth buffer */
				if (z_interpolated < depth_buffer[(height - 1 - y) * width + x])
				{
					depth_buffer[(height - 1 - y) * width + x] = z_interpolated;

					/* Calculate the interpolation value of each data */
					auto interpolated_color = alpha * v0.color + beta * v1.color + gamma * v2.color;
					auto interpolated_normal = alpha * v0.normal + beta * v1.normal + gamma * v2.normal;
					auto interpolated_texture_coordinate = alpha * v0.texture + beta * v1.texture + gamma * v2.texture;
					auto interpolated_shading_point = alpha * view_pos[0] + beta * view_pos[1] + gamma * view_pos[2];

					/* Load shading data */
					Shader shader;
					shader.color = interpolated_color;
					shader.normal = glm::normalize(interpolated_normal);
					shader.texture_coordinate = interpolated_texture_coordinate;
					shader.shading_point = interpolated_shading_point;

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
	for (Triangle trangle : model.faces)
	{
		Vertex a = trangle.vertex1;
		Vertex b = trangle.vertex2;
		Vertex c = trangle.vertex3;

		/* Transform shading point */
		std::array<glm::vec4, 3> mm{this->view * this->model * a.getHomogeneous(),
									this->view * this->model * b.getHomogeneous(),
									this->view * this->model * c.getHomogeneous()};

		std::array<glm::vec3, 3> viewspace_pos;

		std::transform(mm.begin(), mm.end(), viewspace_pos.begin(), [](const glm::vec4& v) { return glm::vec3(v); });

		/* Transform the normal vector */
		Matrix4f inv_trans = glm::transpose(glm::inverse(this->view * this->model));
		Vector4f n[] = {inv_trans * glm::vec4(a.normal, 0.0f),
						inv_trans * glm::vec4(b.normal, 0.0f),
						inv_trans * glm::vec4(c.normal, 0.0f)};
		a.normal = Vector3f(n[0]);
		b.normal = Vector3f(n[1]);
		c.normal = Vector3f(n[2]);

		Vector4f temp_a = mvp * a.getHomogeneous();
		Vector4f temp_b = mvp * b.getHomogeneous();
		Vector4f temp_c = mvp * c.getHomogeneous();

		temp_a /= a.w();
		temp_b /= b.w();
		temp_c /= c.w();

		temp_a.x = 0.5 * width * (temp_a.x + 1.0);
		temp_a.y = 0.5 * height * (temp_a.y + 1.0);
		temp_b.x = 0.5 * width * (temp_b.x + 1.0);
		temp_b.y = 0.5 * height * (temp_b.y + 1.0);
		temp_c.x = 0.5 * width * (temp_c.x + 1.0);
		temp_c.y = 0.5 * height * (temp_c.y + 1.0);

		a.color = {148.0 / 255.0, 121.0 / 255.0, 92.0 / 255.0};
		b.color = {148.0 / 255.0, 121.0 / 255.0, 92.0 / 255.0};
		c.color = {148.0 / 255.0, 121.0 / 255.0, 92.0 / 255.0};

		a.position = Vector3f{temp_a};
		b.position = Vector3f{temp_b};
		c.position = Vector3f{temp_c};

		Triangle temp{a, b, c};
		drawShaderTriangle(temp, viewspace_pos, model.texture);
	}
}

void Rasterizer::clear()
{
	std::fill(screen_buffer.begin(), screen_buffer.end(), Vector3f{0, 0, 0});
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
