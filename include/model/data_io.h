#pragma once

#include <iostream>
#include <sstream>
#include <vector>

#include <camera.h>
#include <material.h>
#include <object.h>
#include <scene.h>
#include <vertex.h>

class InputOutput
{
public:
	InputOutput() = default;

	InputOutput(const std::string& name);

	void loadObjFile(const std::string& path);

	void loadXmlFile(const std::string& path);

	void loadGlbFile(const std::string& path);

	void generateScene(Scene& scene);


	std::string name;
	Camera camera;
	std::vector<Object> objects;
	std::vector<Material> materials;
	std::vector<Texture> textures;
	std::vector<PointLight> point_lights;
	std::map<std::string, Index> texture_index;
};
