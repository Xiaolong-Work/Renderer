#pragma once
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <light.h>
#include <texture.h>
#include <triangle.h>
#include <utils.h>
#include <vertex.h>

class Model
{
public:
	std::vector<Triangle> faces;
	std::vector<PointLight> lights;
	Texture texture;

	Model();
	Model(const std::string& object_filename);
	Model(const std::string& object_filename, const std::string& texture_filepath);

	/* Loading the model */
	bool loadObject(const std::string& object_filename);

	/* Loading the texture */
	bool loadTexture(const std::string& texture_filepath);

	bool texture_flag = false;

	/* Clear data */
	void clear();

private:
	std::vector<Vector3f> vertexs;
	std::vector<Vector3f> normals;
	std::vector<Vector2f> textures;

	bool vertex_loaded = false;
	bool texture_loaded = false;
	bool normal_loaded = false;
};