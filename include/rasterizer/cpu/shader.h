#pragma once

#include <texture.h>
#include <utils.h>

struct light
{
	Vector3f position;
	Vector3f intensity;
};

class Shader
{
public:
	Shader::Shader();
	Shader::Shader(Vector3f color, Vector3f normal, Vector2f texture_coordinate, Texture* texture);

	Vector3f shading_point;
	Vector3f color;
	Vector3f normal;
	Vector2f texture_coordinate;
	Texture* texture;

	/* Normal shading */
	static Vector3f normalFragmentShader(Shader shader);

	/* Texture coloring */
	static Vector3f textureFragmentShader(Shader shader);

	/* Phong shading model */
	static Vector3f phongFragmentShader(Shader payload);
};
