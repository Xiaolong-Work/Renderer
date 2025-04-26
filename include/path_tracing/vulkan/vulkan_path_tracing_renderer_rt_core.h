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

	void recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index) override;
	void updateUniformBufferObjects(const int index) override
	{
		UBOMVP temp;
		temp.model = this->ubo.project * this->ubo.view;
		temp.view = glm::inverse(this->ubo.view);
		temp.project = glm::inverse(this->ubo.project);

		this->uniform_buffer_managers[index].update(&temp);
	}

	void setupGraphicsPipelines();

	void createFrameBuffers()
	{
		this->buffers.resize(this->swap_chain_manager.views.size());

		for (size_t i = 0; i < this->swap_chain_manager.views.size(); i++)
		{
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.pNext = nullptr;
			framebufferInfo.flags = 0;
			framebufferInfo.renderPass = pass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &swap_chain_manager.views[i];
			framebufferInfo.width = swap_chain_manager.extent.width;
			framebufferInfo.height = swap_chain_manager.extent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(context_manager.device, &framebufferInfo, nullptr, &buffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create framebuffer!");
			}
		}
	}

	std::array<DescriptorManager, MAX_FRAMES_IN_FLIGHT> present_descriptor_managers{};
	void setupPresentDescriptor(const int index)
	{
		this->present_descriptor_managers[index].addLayoutBinding(
			this->storage_image_managers[index].getLayoutBinding(0, VK_SHADER_STAGE_FRAGMENT_BIT));
	}

	void setupPresentPipelines()
	{
		this->present_pipeline_manager.setDefaultFixedState();

		this->present_pipeline_manager.addShaderStage("path_tracing_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		this->present_pipeline_manager.addShaderStage("path_tracing_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		std::vector<VkDescriptorSetLayout> layout = {this->present_descriptor_managers[0].layout};
		this->present_pipeline_manager.setDescriptorSetLayout(layout);
		this->present_pipeline_manager.setExtent(swap_chain_manager.extent);
		this->present_pipeline_manager.setRenderPass(pass, 0);

		this->present_pipeline_manager.enable_vertex_inpute = false;
	}

	void setupPresentPass()
	{
	}

	PipelineManager present_pipeline_manager{};

	std::vector<VkFramebuffer> buffers;
	VkRenderPass pass;

protected:
	void getFeatureProperty();

	void createAcceleration();

	void createBLAS();

	void createTLAS();

	void setupDescriptor(const int index);

	void createShaderBindingTable();

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

	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;

	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;

	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;

	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;

	std::array<UniformBufferManager, MAX_FRAMES_IN_FLIGHT> uniform_buffer_managers{};

	std::array<DescriptorManager, MAX_FRAMES_IN_FLIGHT> descriptor_managers{};

	PipelineManager pipeline_manager{};

	VkStridedDeviceAddressRegionKHR ray_generate_region{};
	VkStridedDeviceAddressRegionKHR ray_miss_region{};
	VkStridedDeviceAddressRegionKHR ray_hit_region{};
	VkStridedDeviceAddressRegionKHR call_region{};

	std::array<StorageImageManager, 2> storage_image_managers{};

	StagingBufferManager sbt_buffer_manager{};

	std::vector<VkDescriptorImageInfo> result_images{};
	VkWriteDescriptorSetAccelerationStructureKHR write{};
};