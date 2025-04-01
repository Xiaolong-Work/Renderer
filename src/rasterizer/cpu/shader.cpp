#include <shader.h>
#include <rasterizer.h>

int getCubemapFace(Vector3f dir)
{
	Vector3f absDir = abs(dir);
	if (absDir.x >= absDir.y && absDir.x >= absDir.z)
	{
		return (dir.x > 0) ? 0 : 1; // +X, -X
	}
	else if (absDir.y >= absDir.x && absDir.y >= absDir.z)
	{
		return (dir.y > 0) ? 2 : 3; // +Y, -Y
	}
	else
	{
		return (dir.z > 0) ? 4 : 5; // +Z, -Z
	}
}

Shader::Shader()
{
	this->texture = nullptr;
}
Shader::Shader(Vector3f normal, Vector2f texture_coordinate, Texture* texture)
{
	this->normal = normal;
	this->texture_coordinate = texture_coordinate;
	this->texture = texture;
}

Vector3f Shader::normalFragmentShader(Shader shader, const std::vector<std::vector<std::vector<float>>>& shadow_maps)
{
	return (glm::normalize(shader.normal) + Vector3f(1.0f, 1.0f, 1.0f)) / 2.f;
}

Vector3f Shader::textureFragmentShader(Shader shader, const std::vector<std::vector<std::vector<float>>>& shadow_maps)
{
	Vector3f return_color = {0.5, 0.5, 0.5};
	return return_color;
	if (shader.texture)
	{
		return_color = shader.texture->getColor(shader.texture_coordinate.x, shader.texture_coordinate.y);
	}
	Vector3f texture_color = {return_color.x, return_color.y, return_color.z};

	if (shader.lights.empty())
	{
		return texture_color;
	}

	Vector3f ka = Vector3f(0.005, 0.005, 0.005);
	Vector3f kd = texture_color;
	Vector3f ks = Vector3f(0, 0, 0);

	Vector3f amb_light_intensity{10, 10, 10};
	Vector3f eye_pos{0, 0, 0};

	float p = 150;

	Vector3f color = texture_color;
	Vector3f point = shader.shading_point;
	Vector3f normal = shader.normal;

	Vector3f result_color = {0, 0, 0};

	for (size_t i = 0; i < shader.lights.size(); i++)
	{
		auto& light = shader.lights[i];
		auto& light_position = light.position;

		Vector3f D = normalize(shader.world_point - light_position);
		auto index = getCubemapFace(D);

		auto view = glm::lookAt(light_position, light_position + shader.looks[index], shader.ups[index]);
		auto project = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

		Vector4f P_light = project * view * Vector4f(shader.world_point, 1.0);
		P_light /= P_light.w;

		int x = (P_light.x * 0.5 + 0.5) * 1024;
		int y = (P_light.y * 0.5 + 0.5) * 1024;

		float shadowDepth = shadow_maps[i][index][x * 1024 + y];

		float bias = 0.05;
		int con = (P_light.z - bias) < shadowDepth ? 1.0 : 0.3;

		auto l = glm::normalize(light.position - point);
		auto v = glm::normalize(eye_pos - point);
		auto n = normal;

		auto r2 = glm::dot((light.position - point), (light.position - point));
		auto Ld = kd * ((light.intensity) / r2) * std::max(0.0f, glm::dot(normal, l));

		auto h = glm::normalize(v + l);
		auto Ls = ks * ((light.intensity) / r2) * pow(std::max(0.0f, glm::dot(normal, h)), p);

		result_color += Ls + Ld;

		result_color *= con;
	}
	auto La = ka * (amb_light_intensity);
	result_color += La;

	return result_color;
}
