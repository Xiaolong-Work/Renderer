#include <shader.h>

Shader::Shader()
{
	texture = nullptr;
}
Shader::Shader(Vector3f color, Vector3f normal, Vector2f texture_coordinate, Texture* texture)
{
	this->color = color;
	this->normal = normal;
	this->texture_coordinate = texture_coordinate;
	this->texture = texture;
}

Vector3f Shader::normalFragmentShader(Shader shader)
{
	Vector3f return_color = (glm::normalize(shader.normal) + Vector3f(1.0f, 1.0f, 1.0f)) / 2.f;
	Vector3f result;
	result = {return_color.x * 255, return_color.y * 255, return_color.z * 255};
	return result;
}

Vector3f Shader::textureFragmentShader(Shader shader)
{
	Vector3f return_color = {0, 0, 0};
	if (shader.texture)
	{
		return_color = shader.texture->getColor(shader.texture_coordinate.x, shader.texture_coordinate.y);
	}
	Vector3f texture_color;
	texture_color = {return_color.x, return_color.y, return_color.z};

	Vector3f ka = Vector3f(0.005, 0.005, 0.005);
	Vector3f kd = texture_color / 255.f;
	Vector3f ks = Vector3f(0.7937, 0.7937, 0.7937);

	auto l1 = light{{20, 20, 20}, {500, 500, 500}};
	auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

	std::vector<light> lights = {l1, l2};
	Vector3f amb_light_intensity{10, 10, 10};
	Vector3f eye_pos{0, 0, 10};

	float p = 150;

	Vector3f color = texture_color;
	Vector3f point = shader.shading_point;
	Vector3f normal = shader.normal;

	Vector3f result_color = {0, 0, 0};

	for (auto& light : lights)
	{
		auto l = glm::normalize(light.position - point);
		auto v = glm::normalize(eye_pos - point);
		auto n = normal;

		auto r2 = glm::dot((light.position - point), (light.position - point));
		auto Ld = kd * ((light.intensity) / r2) * std::max(0.0f, glm::dot(normal, l));

		auto h = glm::normalize(v + l);
		auto Ls = ks * ((light.intensity) / r2) * pow(std::max(0.0f, glm::dot(normal, h)), p);

		auto La = ka * (amb_light_intensity);

		result_color += Ls + Ld;
	}
	auto La = ka * (amb_light_intensity);
	result_color += La;

	return result_color * 255.f;
}

Vector3f Shader::phongFragmentShader(Shader payload)
{
	Vector3f ka = Vector3f(0.005, 0.005, 0.005);
	Vector3f kd = payload.color;
	Vector3f ks = Vector3f(0.7937, 0.7937, 0.7937);

	auto l1 = light{{20, 20, 20}, {500, 500, 500}};
	auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

	std::vector<light> lights = {l1, l2};
	Vector3f amb_light_intensity{10, 10, 10};
	Vector3f eye_pos{0, 0, 5};

	float p = 150;

	Vector3f color = payload.color;
	Vector3f point = payload.shading_point;
	Vector3f normal = payload.normal;

	Vector3f result_color = {0, 0, 0};
	for (auto& light : lights)
	{
		auto l = glm::normalize(light.position - point);
		auto v = glm::normalize(eye_pos - point);
		auto n = normal;

		auto r2 = glm::dot((light.position - point), (light.position - point));
		auto Ld = kd * ((light.intensity) / r2) * std::max(0.0f, glm::dot(normal, l));

		auto h = glm::normalize(v + l);
		auto Ls = ks * ((light.intensity) / r2) * pow(std::max(0.0f, glm::dot(normal, h)), p);

		auto La = ka * (amb_light_intensity);

		result_color += Ls + Ld;
	}
	auto La = ka * (amb_light_intensity);
	result_color += La;

	return result_color * 255.f;
}