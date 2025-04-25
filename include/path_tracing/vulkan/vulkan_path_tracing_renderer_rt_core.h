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
#include <vulkan_render_base.h>

class VulkanPathTracingRendererRTCore : public VulkanRendererBase
{
public:
	VulkanPathTracingRendererRTCore();
	~VulkanPathTracingRendererRTCore();

	void init();
	void clear();

	void setData(const Scene& scene);

	void recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index) override
	{
	}
	void updateUniformBufferObjects(int index) override
	{
	}

	void setupGraphicsPipelines();
	

protected:
	void getFeatureProperty();

	void createAcceleration();

	void createBLAS();

	void createTLAS();

	void setupDescriptor(const int index);

private:
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_property{};

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing_feature{};

	VkPhysicalDeviceAccelerationStructurePropertiesKHR acceleration_property{};

	VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration_feature{};

	std::vector<VkAccelerationStructureKHR> blas{};
	VkAccelerationStructureKHR tlas;

	std::vector<VertexBufferManager> all_vertex_managers{};
	std::vector<IndexBufferManager> all_index_managers{};

	StorageBufferManager all_blas_buffer_manager{};

	StorageBufferManager instance_buffer_manager{};

	StorageBufferManager tlas_buffer_manager{};

	StorageBufferManager scratch_buffer_manager{};

	std::vector<VkTransformMatrixKHR> model_matrixes{};

	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;

	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;

	std::array<UniformBufferManager, MAX_FRAMES_IN_FLIGHT> uniform_buffer_managers{};

	std::array<DescriptorManager, MAX_FRAMES_IN_FLIGHT> descriptor_managers{};

	PipelineManager pipeline_manager{};
};