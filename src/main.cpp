#pragma once
#include <chrono>
#include <map>

#define GLFW_INCLUDE_VULKAN
#include <data_io.h>
#include <data_loader.h>
#include <GLFW/glfw3.h>
#include <path_tracing_scene.h>
#include <render.h>
#include <renderer.h>
#include <scene.h>
#include <utils.h>

#include <cpu_rasterizer_renderer.h>
#include <vulkan_path_tracing_renderer_rt_core.h>
#include <vulkan_rasterizer_render.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

std::map<int, std::string> name = {{0, "cornell-box"}, {1, "veach-mis"}, {2, "bathroom"}};

void rasterRenderCPU()
{
	CPURasterizerRenderer r{};
	r.init();
	r.draw();
	r.clear();
}

void rasterRender(const Scene& scene)
{
	VulkanRasterRenderer renderer;
	renderer.setData(scene);
	try
	{
		renderer.run();
	} catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
}

void pathTracingGPU(const PathTracingScene& scene, const int spp)
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
	VulkanPathTracingRendererRTCore renderer;
}

void pathTracingRender()
{
	int spp = 4;
	int max_depth = 5;

	Renderer renderer;

	PathTracingScene scene;

	int scene_index = 2;
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
	// renderer.render(scene);
	end = std::chrono::system_clock::now();
	outputTimeUse("Render CPU", end - start);

	return;
}

int main()
{
	// rasterRenderCPU();

	// pathTracingRender();
	// pathTracingRTCore();

	// std::string path = std::string(ROOT_DIR) + "/models/bathroom/";
	// InputOutput io{"bathroom"};
	// io.loadObjFile(path);
	// io.loadXmlFile(path);

	// std::string path = std::string(ROOT_DIR) + "/models/viking_room/";
	// InputOutput io{"viking_room"};
	// io.loadObjFile(path);

	std::string path = std::string(ROOT_DIR) + "/models/light_test/";
	InputOutput io{"light_test"};
	io.loadObjFile(path);

	Scene scene;
	io.generateScene(scene);

	rasterRender(scene);

	return 0;
}