#pragma once

#include <iostream>
#include <sstream>
#include <vector>

#include <tiny_obj_loader.h>
#include <tinyxml2.h>

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

	void loadObj(const std::string& path);

	void loadXmlFile(const std::string& path);

	void generateScene(Scene& scene);

private:
	std::string name;
	Camera camera;
	std::vector<Object> objects;
	std::vector<Material> materials;
	std::map<std::string, Index> texture_index;
};
