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
#include <vulkan_deferred_render.h>
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
	// VulkanRasterDeferredRenderer renderer;
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

void pathTracingRTCore(const Scene& scene)
{
	VulkanPathTracingRendererRTCore renderer{};
	renderer.setWindowSize(scene);
	renderer.init();
	renderer.setData(scene);
	try
	{
		renderer.run();
	} catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
}

void pathTracingRender()
{
	int spp = 4;
	int max_depth = 5;

	Renderer renderer;

	PathTracingScene scene;

	int scene_index = 0;
	scene.name = name[scene_index];
	scene.max_depth = max_depth;
	renderer.spp = spp;

	auto start = std::chrono::system_clock::now();
	std::string path = std::string(ROOT_DIR) + "/models/" + name[scene_index] + "/";
	InputOutput io(name[scene_index]);
	io.loadObjFile(path);
	io.loadXmlFile(path);
	Scene temp_scene;
	io.generateScene(temp_scene);
	scene = temp_scene;

	auto end = std::chrono::system_clock::now();
	outputTimeUse("Load Data", end - start);

	scene.setData(io.objects, io.camera);

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
	//rasterRenderCPU();

	//pathTracingRender();

	// std::string path = std::string(ROOT_DIR) + "/models/viking_room/";
	// InputOutput io{"viking_room"};
	// io.loadObjFile(path);
	// pathTracingRender();

	// return 0;

	std::string path1 = std::string(ROOT_DIR) + "/models/minecraft/";
	InputOutput io1{"mill"};
	Vector3f torches_light_color{1, 0.6875, 0.6875};
	float torches_light_intensity{150};
	io1.point_lights.push_back(PointLight{{-2, 13, -17}, {1.0f, 1.0f, 1.0f}, 100000.0f});
	io1.point_lights.push_back(PointLight{{0.5, -7.4, 7.5}, torches_light_color, torches_light_intensity});
	io1.point_lights.push_back(PointLight{{4.5, -7.4, 12.5}, torches_light_color, torches_light_intensity});
	io1.point_lights.push_back(PointLight{{8.5, -7.4, 9.5}, torches_light_color, torches_light_intensity});
	io1.point_lights.push_back(PointLight{{4.5, -12.4, 12.5}, torches_light_color, torches_light_intensity});
	io1.point_lights.push_back(PointLight{{8.5, -12.4, 7.5}, torches_light_color, torches_light_intensity});
	io1.point_lights.push_back(PointLight{{4.5, -12.4, 3.5}, torches_light_color, torches_light_intensity});
	io1.point_lights.push_back(PointLight{{0.5, -12.4, 7.5}, torches_light_color, torches_light_intensity});
	io1.point_lights.push_back(PointLight{{3.5, -17.4, 11.5}, torches_light_color, torches_light_intensity});
	io1.point_lights.push_back(PointLight{{1.5, -17.4, 6.5}, torches_light_color, torches_light_intensity});
	io1.point_lights.push_back(PointLight{{7.5, -17.4, 5.5}, torches_light_color, torches_light_intensity});
	io1.point_lights.push_back(PointLight{{-10.5, -15.4, -1.5}, torches_light_color, torches_light_intensity});
	io1.loadGlbFile(path1);
	io1.camera.fovy = 45.0;
	io1.camera.position = Vector3f(0, 0, 0);
	io1.camera.look = Vector3f(0, 0, 1);
	io1.camera.up = Vector3f(0, 1, 0);
	// std::string path = std::string(ROOT_DIR) + "/models/light_test/";
	// InputOutput io{"light_test"};
	// io.loadGlbFile(path);
	Scene scene1;
	io1.generateScene(scene1);
	//rasterRender(scene1);

	int scene_index = 0;
	std::string path = std::string(ROOT_DIR) + "/models/" + name[scene_index] + "/";
	InputOutput io(name[scene_index]);
	io.loadObjFile(path);
	io.loadXmlFile(path);
	if (scene_index == 2)
	{
		for (auto& vertex : io.objects[3].vertices)
		{
			vertex.normal *= -1;
		}
		for (auto& vertex : io.objects[4].vertices)
		{
			vertex.normal *= -1;
		}
	}

	io.point_lights.push_back(PointLight{{-2, 13, -17}, {1.0f, 1.0f, 1.0f}, 100000.0f});
	Scene scene;
	io.generateScene(scene);
	pathTracingRTCore(scene);

	return 0;
}