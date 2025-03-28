#include <shader.h>

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

Vector3f Shader::normalFragmentShader(Shader shader)
{
	return (glm::normalize(shader.normal) + Vector3f(1.0f, 1.0f, 1.0f)) / 2.f;
}

Vector3f Shader::textureFragmentShader(Shader shader)
{
	Vector3f return_color = {0, 0, 0};
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
	Vector3f kd = texture_color / 255.f;
	Vector3f ks = Vector3f(0.7937, 0.7937, 0.7937);

	Vector3f amb_light_intensity{10, 10, 10};
	Vector3f eye_pos{0, 0, 10};

	float p = 150;

	Vector3f color = texture_color;
	Vector3f point = shader.shading_point;
	Vector3f normal = shader.normal;

	Vector3f result_color = {0, 0, 0};

	for (auto& light : shader.lights)
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

	return result_color;
}
