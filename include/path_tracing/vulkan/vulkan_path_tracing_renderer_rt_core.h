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

#include <scene.h>

class VulkanPathTracingRendererRTCore
{
public:
	VulkanPathTracingRendererRTCore();
	~VulkanPathTracingRendererRTCore();

	void init();
	void clear();

protected:
	void getFeatureProperty();

	void createAcceleration();

private:
	ContextManager context_manager{};

	CommandManager command_manager{};

	SwapChainManager swap_chain_manager{};

	TextureManager texture_manager{};

	

	SwapChainManager swapChianManager{};

	VertexBufferManager vertexBufferManager{};

	IndexBufferManager indexBufferManager{};

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