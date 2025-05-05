
#include <rasterizer.h>
#include <stb_image_write.h>

void saveDepth(const std::vector<float>& data, const int index)
{
	std::string path = std::string(ROOT_DIR) + "/results/" + std::to_string(index) + ".bmp";

	std::vector<unsigned char> image_data(1024 * 1024 * 3);

	float min = std::numeric_limits<float>::infinity();
	float max = 0;

	for (auto& index : data)
	{
		if (index < min)
		{
			min = index;
		}
		if (index > max && index < std::numeric_limits<float>::infinity())
		{
			max = index;
		}
	}

	auto length = max - min;

	for (auto i = 0; i < 1024 * 1024; ++i)
	{
		static unsigned char color[3];
		image_data[i * 3 + 0] = (unsigned char)(255 * (data[i] - min) / length);
		image_data[i * 3 + 1] = (unsigned char)(255 * (data[i] - min) / length);
		image_data[i * 3 + 2] = (unsigned char)(255 * (data[i] - min) / length);
	}

	stbi_write_bmp(path.c_str(), 1024, 1024, 3, image_data.data());
}

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
	this->g_buffer.resize(width * height);
	this->shadow_maps.resize(this->lights.size());
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
	for (int i = 0; i < model.faces.size(); i++)
	{
		auto& triangle = model.faces[1];

		Vector4f a = getHomogeneous(triangle.vertex1.position);
		Vector4f b = getHomogeneous(triangle.vertex2.position);
		Vector4f c = getHomogeneous(triangle.vertex3.position);

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
									const std::array<Point, 3> world_position,
									Texture* texture,
									const std::vector<PointLight>& lights)
{
	auto& v0 = position[0];
	auto& v1 = position[1];
	auto& v2 = position[2];

	/* Calculate the bounding box of the triangle */
	int x_max = std::max(v0.x, std::max(v1.x, v2.x));
	int x_min = std::ceil(std::min(v0.x, std::min(v1.x, v2.x)));
	int y_max = std::ceil(std::max(v0.y, std::max(v1.y, v2.y)));
	int y_min = std::min(v0.y, std::min(v1.y, v2.y));

	x_max = std::min(x_max, width - 1);
	x_min = std::max(0, x_min);
	y_max = std::min(y_max, height - 1);
	y_min = std::max(0, y_min);

	/* For each pixel in the bounding box, determine whether it is inside the triangle and rasterize the triangle */
#pragma omp parallel for collapse(2)
	for (int y = y_min; y <= y_max; y++)
	{
		for (int x = x_min; x <= x_max; x++)
		{
			/* Check if it is inside the triangle */
			if (isInsideTriangle(x, y, position))
			{
				/* Get the center of gravity coordinates */
				auto barycentric = get2DBarycentric(x, y, position);
				float alpha = barycentric.x;
				float beta = barycentric.y;
				float gamma = barycentric.z;

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
					Vector3f interpolated_world_point =
						(alpha * (world_position[0] / v0.w) + beta * (world_position[1] / v1.w) +
						 gamma * (world_position[2] / v2.w)) *
						w_reciprocal;

					GBuffer buffer;
					buffer.normal = glm::normalize(interpolated_normal);
					buffer.texture_coordinate = interpolated_texture_coordinate;
					buffer.shading_point = interpolated_shading_point;
					buffer.world_point = interpolated_world_point;
					buffer.flag = true;
					this->g_buffer[(height - 1 - y) * width + x] = buffer;
				}
			}
		}
	}
}

void Rasterizer::drawShaderTriangleframe(Model model)
{
	//updateShadowMaps(model);

	Matrix4f mvp = this->projection * this->view * this->model;
	Matrix4f mv = this->view * this->model;

	std::vector<std::array<Vector4f, 3>> positions;
	std::vector<std::array<Direction, 3>> normals;
	std::vector<std::array<Coordinate2D, 3>> texture_coordinates;
	std::vector<std::array<Point, 3>> viewspace_positions;
	std::vector<std::array<Point, 3>> world_positions;

	size_t size = model.faces.size();
	positions.resize(size);
	normals.resize(size);
	texture_coordinates.resize(size);
	viewspace_positions.resize(size);
	world_positions.resize(size);

#pragma omp parallel for
	for (int i = 0; i < size; i++)
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
			return Vector3f(inverse_transform * Vector4f(normal, 1.0f));
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

		auto get_world_coordinate = [this](const Point& point) {
			return Vector3f(this->model * Vector4f(point, 1.0f));
		};
		world_positions[i] = {
			get_world_coordinate(a.position), get_world_coordinate(b.position), get_world_coordinate(c.position)};

		texture_coordinates[i] = {a.texture, b.texture, c.texture};
	}

	Texture* temp = &model.texture;
	if (!model.texture_flag)
	{
		temp = nullptr;
	}

	std::vector<PointLight> lights;
	for (auto& light : model.lights)
	{
		auto temp = getHomogeneous(light.position);

		// temp = mv * temp;
		// temp /= temp.w;

		lights.push_back(PointLight{Vector3f(temp), light.color});
	}

	for (size_t i = 0; i < size; i++)
	{
		drawShaderTriangle(
			positions[i], normals[i], texture_coordinates[i], viewspace_positions[i], world_positions[i], temp, lights);
	}

	/* Deferred Rendering */
#pragma omp parallel for collapse(2)
	for (int y = 0; y <= height - 1; y++)
	{
		for (int x = 0; x <= width - 1; x++)
		{
			auto& buffer = this->g_buffer[(height - 1 - y) * width + x];

			if (!buffer.flag)
			{
				continue;
			}

			/* Load shading data */
			Shader shader{};
			shader.normal = buffer.normal;
			shader.texture_coordinate = buffer.texture_coordinate;
			shader.shading_point = buffer.shading_point;
			shader.world_point = buffer.world_point;
			shader.texture = temp;
			shader.lights = lights;

			/* Shader calculation shading */
			Vector3f ka = Vector3f(0.005, 0.005, 0.005);
			Vector3f kd = Vector3f(0.6);
			Vector3f ks = Vector3f(0, 0, 0);

			Vector3f amb_light_intensity{10, 10, 10};
			Vector3f eye_pos{0, 0, 0};

			float p = 150;

			Vector3f color = Vector3f(0);
			Vector3f point = shader.shading_point;
			Vector3f normal = shader.normal;

			Vector3f result_color = {0, 0, 0};

			float con = 0;
			int count = 0;
			for (size_t i = 0; i < shader.lights.size(); i++)
			{
				auto& light = shader.lights[i];
				auto& light_position = light.position;

				Vector3f point_to_light = normalize(shader.world_point - light_position);
				auto faceIndex = getCubemapFace(point_to_light);

				auto view = glm::lookAt(light_position, light_position + looks[faceIndex], ups[faceIndex]);
				auto project = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 20.0f);

				Vector4f P_light = project * view * Vector4f(shader.world_point, 1.0);
				P_light /= P_light.w;

				int x = (P_light.x + 1.0) * 0.5 * (1024 - 1);
				int y = (P_light.y + 1.0) * 0.5 * (1024 - 1);

				
				for (int k = std::max(0, x - 2); k <= std::min(1023, x + 2); k++)
				{
					for (int l = std::max(0, y - 2); l <= std::min(1023, y + 2); l++)
					{
						float shadowDepth = shadow_maps[i][faceIndex][(height - 1 - l) * width + k];
						float bias = 0.0001;
						con += (P_light.z - bias) < shadowDepth ? 1.0f : 0.1f;
						count++;
					}
				}
				

				//float shadowDepth = shadow_maps[i][faceIndex][(height - 1 - y) * width + x];
				//float bias = 0.0001 * (1.0 - P_light.z / 20.0f);
				//float con = (P_light.z - bias) < shadowDepth ? 1.0f : 0.1f;

				auto view_light_position = Vector3f(this->view * Vector4f(light_position, 1.0));

				auto l = glm::normalize(view_light_position - point);
				auto v = glm::normalize(eye_pos - point);
				auto n = normal;

				auto r2 = glm::dot((view_light_position - point), (view_light_position - point));
				auto Ld = kd * ((light.intensity) / r2) * std::max(0.0f, glm::dot(normal, l));

				auto h = glm::normalize(v + l);
				auto Ls = ks * ((light.intensity) / r2) * pow(std::max(0.0f, glm::dot(normal, h)), p);

				result_color += Ls + Ld;

				
			}

			con /= (float)count;
			result_color *= con;

			auto La = ka * (amb_light_intensity);
			result_color += La;

			if (count == 0)
			{
				result_color = temp->getColor(buffer.texture_coordinate.x, buffer.texture_coordinate.y);
			}

			Vector2i c{x, y};
			// setPixel(point, pixel_color);
			setPixel(c, result_color);

			
		}

		
	}
}

void Rasterizer::clear()
{
	std::fill(screen_buffer.begin(), screen_buffer.end(), Vector4f{0, 0, 0, 0});
	std::fill(depth_buffer.begin(), depth_buffer.end(), std::numeric_limits<float>::infinity());
	std::fill(g_buffer.begin(), g_buffer.end(), GBuffer{});
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

void Rasterizer::updateShadowMaps(Model model)
{
	/* For each light */
	for (size_t i = 0; i < this->shadow_maps.size(); i++)
	{
		/* For each face */
		for (size_t j = 0; j < 6; j++)
		{
			auto& shadow_map = this->shadow_maps[i][j];
			std::fill(shadow_map.begin(), shadow_map.end(), std::numeric_limits<float>::infinity());

			auto& light_position = this->lights[i].position;

			auto view = glm::lookAt(light_position, light_position + looks[j], ups[j]);
			auto project = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 20.0f);

			auto mvp = project * view * this->model;

			std::vector<std::array<Vector4f, 3>> positions;

			size_t size = model.faces.size();
			positions.resize(size);

#pragma omp parallel for
			for (int i = 0; i < size; i++)
			{
				auto& triangle = model.faces[i];

				auto& a = triangle.vertex1;
				auto& b = triangle.vertex2;
				auto& c = triangle.vertex3;

				/* Transform the position */
				auto trans_position = [mvp, this](const Point point) {
					Vector4f result = mvp * this->getHomogeneous(point);
					result /= result.w;
					result.x = (result.x + 1.0) * 0.5 * (1024 - 1);
					result.y = (result.y + 1.0) * 0.5 * (1024 - 1);
					return result;
				};
				positions[i] = {trans_position(a.position), trans_position(b.position), trans_position(c.position)};
			}

			for (int i = 0; i < size; i++)
			{
				auto& position = positions[i];

				auto& v0 = position[0];
				auto& v1 = position[1];
				auto& v2 = position[2];

				/* Calculate the bounding box of the triangle */
				int x_max = std::ceil(std::max(v0.x, std::max(v1.x, v2.x)));
				int x_min = std::ceil(std::min(v0.x, std::min(v1.x, v2.x)));
				int y_max = std::ceil(std::max(v0.y, std::max(v1.y, v2.y)));
				int y_min = std::ceil(std::min(v0.y, std::min(v1.y, v2.y)));

				x_max = std::min(x_max, 1023);
				x_min = std::max(0, x_min);
				y_max = std::min(y_max, 1023);
				y_min = std::max(0, y_min);

				/* For each pixel in the bounding box, determine whether it is inside the triangle and rasterize the
				 * triangle */
#pragma omp parallel for collapse(2)
				for (int y = y_min; y <= y_max; y++)
				{
					for (int x = x_min; x <= x_max; x++)
					{
						/* Check if it is inside the triangle */
						if (isInsideTriangle(x, y, position))
						{
							/* Get the center of gravity coordinates */
							auto barycentric = get2DBarycentric(x, y, position);
							float alpha = barycentric.x;
							float beta = barycentric.y;
							float gamma = barycentric.z;

							/* Perform interpolation on depth */
							float z_interpolated = alpha * v0.z + beta * v1.z + gamma * v2.z;

							/* Depth buffer */
							if (z_interpolated < shadow_map[(height - 1 - y) * width + x])
							{
								shadow_map[(height - 1 - y) * width + x] = z_interpolated;
							}
						}
					}
				}
			}
		}
	}
}

void Rasterizer::genetareShadowMaps(Model model)
{
	this->lights = model.lights;
	this->shadow_maps.resize(this->lights.size());

	/* For each light */
	for (size_t i = 0; i < this->shadow_maps.size(); i++)
	{
		this->shadow_maps[i].resize(6);

		/* For each face */
		for (size_t j = 0; j < 6; j++)
		{
			this->shadow_maps[i][j].resize(1024 * 1024, std::numeric_limits<float>::infinity());
		}
	}
}
