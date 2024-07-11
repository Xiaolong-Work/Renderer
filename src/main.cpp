#pragma once
#include <chrono>
#include <map>

#include <data_loader.h>
#include <GLFW/glfw3.h>
#include <render.h>
#include <scene.h>
#include <utils.h>

std::map<int, std::string> name = {{0, "cornell-box"}, {1, "veach-mis"}, {2, "bathroom"}};

int main()
{
	Renderer renderer;
	Scene scene;

	int scene_index = 0;
	scene.name = name[scene_index];
	scene.max_depth = 5;
	renderer.spp = 16;

	auto start = std::chrono::system_clock::now();
	DataLoader loader(std::string(ROOT_DIR) + "/models/" + name[scene_index] + "/", name[scene_index]);
	loader.load();
	auto end = std::chrono::system_clock::now();
	outputTimeUse("Load Data", end - start);

	scene.setData(loader.objects, loader.camera);

	start = std::chrono::system_clock::now();
	scene.initBVH();
	end = std::chrono::system_clock::now();
	outputTimeUse("Build BVH", end - start);

	start = std::chrono::system_clock::now();
	renderer.render(scene);
	end = std::chrono::system_clock::now();
	outputTimeUse("Render", end - start);

	return 0;
}