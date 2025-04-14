#pragma once

#include <model.h>
#include <texture.h>
#include <utils.h>

class Shader
{
public:
	Shader::Shader();
	Shader::Shader(Vector3f normal, Vector2f texture_coordinate, Texture* texture);

	Vector3f shading_point;
	Vector3f world_point;
	Vector3f normal;
	Vector2f texture_coordinate;
	Texture* texture;
	std::vector<PointLight> lights;

	std::vector<Direction> ups = {Direction(0, -1, 0),
								  Direction(0, -1, 0),
								  Direction(0, 0, 1),
								  Direction(0, 0, -1),
								  Direction(0, -1, 0),
								  Direction(0, -1, 0)};

	std::vector<Vector3f> looks{Direction(1, 0, 0),
								Direction(-1, 0, 0),
								Direction(0, 1, 0),
								Direction(0, -1, 0),
								Direction(0, 0, 1),
								Direction(0, 0, -1)};

	/* Normal shading */
	static Vector3f normalFragmentShader(Shader shader,
										 const std::vector<std::vector<std::vector<float>>>& shadow_maps);

	/* Texture coloring */
	static Vector3f textureFragmentShader(Shader shader,
										  const std::vector<std::vector<std::vector<float>>>& shadow_maps);
};
