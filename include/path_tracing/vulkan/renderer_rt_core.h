#pragma once

#include <fstream>
#include <iostream>

#include <buffer_manager.h>
#include <command_manager.h>
#include <content_manager.h>
#include <descriptor_manager.h>
#include <image_manager.h>
#include <pipeline_manager.h>
#include <shader_manager.h>
#include <swap_chain_manager.h>
#include <texture_manager.h>

#include <scene.h>

class VulkanPathTracingRenderRTCore
{
public:
	VulkanPathTracingRenderRTCore();
	~VulkanPathTracingRenderRTCore();

	void init();
	void clear();

protected:
	void getFeatureProperty();

	void createAcceleration();

private:
	ContentManager contentManager{};

	SwapChainManager swapChainManager{};

	TextureManager textureManager{};

	CommandManager commandManager{};

	SwapChainManager swapChianManager{};

	VertexBufferManager vertexBufferManager{};

	IndexBufferManager indexBufferManager{};

	UniformBufferManager uniformBufferManager{};

	DepthImageManager depthImageManager{};

	DescriptorManager descriptorManager{};

	ShaderManager vertexShaderManager{};

	ShaderManager fragmentShaderManager{};

	PipelineManager graphicsPipelineManager{};

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR property{};

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR feature{};
};