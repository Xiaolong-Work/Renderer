#pragma once
#include <chrono>
#include <map>

#define GLFW_INCLUDE_VULKAN
#include <data_loader.h>
#include <GLFW/glfw3.h>
#include <render.h>
#include <renderer.h>
#include <scene.h>
#include <utils.h>

std::map<int, std::string> name = {{0, "cornell-box"}, {1, "veach-mis"}, {2, "bathroom"}};

int main()
{
	int spp = 512;
	int max_depth = 5;

	Renderer renderer;
	VulkanPathTracingRender app{};
	Scene scene;

	int scene_index = 0;
	scene.name = name[scene_index];
	scene.max_depth = max_depth;
	renderer.spp = spp;

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

	app.setSpp(spp);
	app.init(scene);

	start = std::chrono::system_clock::now();
	app.draw();
	end = std::chrono::system_clock::now();
	outputTimeUse("Render GPU", end - start);

	app.saveResult();

	start = std::chrono::system_clock::now();
	renderer.render(scene);
	end = std::chrono::system_clock::now();
	outputTimeUse("Render CPU", end - start);

	return 0;
}