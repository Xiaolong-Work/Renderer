#pragma once

#include <fstream>
#include <iostream>

#include <buffer_manager.h>
#include <command_manager.h>
#include <context_manager.h>
#include <descriptor_manager.h>
#include <image_manager.h>
#include <pipeline_manager.h>
#include <shader_manager.h>
#include <swap_chain_manager.h>
#include <texture_manager.h>

#include <path_tracing_scene.h>

class VulkanPathTracingRendererRTCore
{
public:
	VulkanPathTracingRendererRTCore();
	~VulkanPathTracingRendererRTCore();

	void init();
	void clear();

	void setSceneData(const Scene& scene)
	{
		this->vertex_buffer_manager.vertices = scene.objects[0].vertices;
		this->index_buffer_manager.indices = scene.objects[0].indices;
		this->vertex_buffer_manager.init();
		this->index_buffer_manager.init();
	}

protected:
	void getFeatureProperty();

	void createAcceleration();

	void createBLAS();

private:
	ContextManager context_manager{};

	CommandManager command_manager{};

	VertexBufferManager vertex_buffer_manager{};

	IndexBufferManager index_buffer_manager{};

	SwapChainManager swap_chain_manager{};

	TextureManager texture_manager{};

	SwapChainManager swapChianManager{};

	UniformBufferManager uniformBufferManager{};

	DepthImageManager depthImageManager{};

	DescriptorManager descriptor_manager{};

	ShaderManager vertex_shader_manager{};

	ShaderManager fragment_shader_manager{};

	PipelineManager graphics_pipeline_manager{};

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_property{};

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_feature{};

	VkPhysicalDeviceAccelerationStructurePropertiesKHR acceleration_property{};

	VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_feature{};
};