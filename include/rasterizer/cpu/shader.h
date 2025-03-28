#pragma once

#include <texture.h>
#include <utils.h>

struct Light
{
	Vector3f position;
	Vector3f intensity;
};

class Shader
{
public:
	Shader::Shader();
	Shader::Shader(Vector3f normal, Vector2f texture_coordinate, Texture* texture);

	Vector3f shading_point;
	Vector3f normal;
	Vector2f texture_coordinate;
	Texture* texture;
	std::vector<Light> lights;

	/* Normal shading */
	static Vector3f normalFragmentShader(Shader shader);

	/* Texture coloring */
	static Vector3f textureFragmentShader(Shader shader);
};
