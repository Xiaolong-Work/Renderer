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

#include <renderer_rt_core.h>
#include <vulkan_rasterizer_render.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

std::map<int, std::string> name = {{0, "cornell-box"}, {1, "veach-mis"}, {2, "bathroom"}};

void rasterRender()
{
	VulkanRasterRenderer renderer;
	renderer.run();
}

void pathTracingGPU(const Scene& scene, const int spp)
{
	VulkanPathTracingRender app{};
	app.setSpp(spp);
	app.init(scene);

	auto start = std::chrono::system_clock::now();
	app.draw();
	auto end = std::chrono::system_clock::now();
	outputTimeUse("Render GPU", end - start);

	app.saveResult();
}

void pathTracingRTCore()
{
	VulkanPathTracingRenderRTCore renderer;
}

void pathTracingRender()
{
	int spp = 16;
	int max_depth = 8;

	Renderer renderer;

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

	pathTracingGPU(scene, spp);

	start = std::chrono::system_clock::now();
	renderer.render(scene);
	end = std::chrono::system_clock::now();
	outputTimeUse("Render CPU", end - start);

	return;
}

int main()
{
	// rasterRender();
	pathTracingRender();
	pathTracingRTCore();
}