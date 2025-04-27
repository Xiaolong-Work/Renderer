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

	std::array<DescriptorManager, MAX_FRAMES_IN_FLIGHT> present_descriptor_managers{};
	void setupPresentDescriptor(const int index)
	{
		this->present_descriptor_managers[index].addDescriptor(
			this->storage_image_managers[index].getDescriptor(9, VK_SHADER_STAGE_FRAGMENT_BIT));

		this->present_descriptor_managers[index].init();
	}

	PipelineManager present_pipeline_manager{};
	void setupPresentPipeline()
	{
		std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		this->present_pipeline_manager.dynamic_states = dynamic_states;

		this->present_pipeline_manager.addShaderStage("path_tracing_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		this->present_pipeline_manager.addShaderStage("path_tracing_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		this->present_pipeline_manager.setDefaultFixedState();
		this->present_pipeline_manager.setExtent(this->swap_chain_manager.extent);
		this->present_pipeline_manager.setRenderPass(this->render_pass_manager.pass, 0);
		this->present_pipeline_manager.setVertexInput(0b0000);
		std::vector<VkDescriptorSetLayout> layout = {this->present_descriptor_managers[0].layout};
		this->present_pipeline_manager.setDescriptorSetLayout(layout);

		this->present_pipeline_manager.init();
	}

	void setupPresentRenderPass()
	{
		this->render_pass_manager.type = RenderPassType::Render;
		this->render_pass_manager.init();
	}

	StorageBufferManager object_address_manager{};
	StorageBufferManager object_property_manager{};
	void setupObjectAddress(const Scene& scene)
	{
		struct ObjectAddress
		{
			uint64_t vertex_address;
			uint64_t index_address;
		};

		struct ObjectProperty
		{
			Vector3f radiance{0};
			int is_light{0};
			Vector3f color{1};
			float area{0};
		};

		std::vector<ObjectAddress> all_object_address;
		std::vector<ObjectProperty> all_object_property;

		uint64_t base_vertex_address = this->vertex_buffer_manager.getBufferAddress();
		uint64_t base_index_address = this->index_buffer_manager.getBufferAddress();
		auto& commands = this->indirect_buffer_manager.commands;
		for (size_t i = 0; i < commands.size(); i++)
		{
			auto& object = scene.objects[i];
			auto& command = commands[i];
			uint64_t vertex_address = base_vertex_address + command.vertexOffset * sizeof(Vertex);
			uint64_t index_address = base_index_address + command.firstIndex * sizeof(Index);
			ObjectAddress address{vertex_address, index_address};
			ObjectProperty property{object.radiance, object.is_light, object.radiance, object.area};
			all_object_address.push_back(address);
			all_object_property.push_back(property);
		}

		this->object_address_manager.setData(
			all_object_address.data(), all_object_address.size() * sizeof(ObjectAddress), all_object_address.size());
		this->object_address_manager.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		this->object_address_manager.init();

		this->object_property_manager.setData(all_object_property.data(),
											  all_object_property.size() * sizeof(ObjectProperty),
											  all_object_property.size());
		this->object_property_manager.init();
	}

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