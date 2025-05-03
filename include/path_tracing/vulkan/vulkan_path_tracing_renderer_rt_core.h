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

	int frame_count = 0;

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

		this->last_camera_matrix = this->current_camera_matrix;
		this->current_camera_matrix = this->ubo.project * this->ubo.view;

		this->uniform_buffer_managers[index].update(&temp);
	}

	void setupGraphicsPipelines();

	std::array<DescriptorManager, MAX_FRAMES_IN_FLIGHT> present_descriptor_managers{};
	void setupPresentDescriptor(const int index)
	{
		this->present_descriptor_managers[index].addDescriptors(
			this->geometry_buffer_managers[index].getDescriptors(0, VK_SHADER_STAGE_FRAGMENT_BIT));

		VkDescriptorSetLayoutBinding layout_binding{};
		/* Result image binding */
		layout_binding.binding = 9;
		layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		layout_binding.descriptorCount = 2;
		layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		layout_binding.pImmutableSamplers = nullptr;
		this->present_descriptor_managers[index].addLayoutBinding(layout_binding);

		/* Result image */
		result_images.resize(2);
		for (size_t i = 0; i < 2; i++)
		{
			result_images[i].sampler = this->storage_image_managers[i].sampler;
			result_images[i].imageView = this->storage_image_managers[i].view;
			result_images[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		}
		VkWriteDescriptorSet result_image_write{};
		result_image_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		result_image_write.pNext = nullptr;
		result_image_write.dstSet = VK_NULL_HANDLE;
		result_image_write.dstBinding = 9;
		result_image_write.dstArrayElement = 0;
		result_image_write.descriptorCount = static_cast<uint32_t>(result_images.size());
		result_image_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		result_image_write.pBufferInfo = nullptr;
		result_image_write.pImageInfo = result_images.data();
		result_image_write.pTexelBufferView = nullptr;
		this->present_descriptor_managers[index].addWrite(result_image_write);

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

		struct PushConstantData
		{
			Matrix4f last_camera_matrix;
			int frame_index;
			int frame_count;
		};

		std::vector<VkPushConstantRange> all_push_constants{};
		VkPushConstantRange push_constant{};
		push_constant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		push_constant.offset = 0;
		push_constant.size = sizeof(PushConstantData);
		all_push_constants.push_back(push_constant);

		this->present_pipeline_manager.setLayout(layout, all_push_constants);

		this->present_pipeline_manager.init();
	}

	void setupPresentRenderPass()
	{
		this->render_pass_manager.type = RenderPassType::Render;
		this->render_pass_manager.init();
	}

	StorageBufferManager object_luminous_indices_manager{};
	StorageBufferManager object_address_manager{};
	StorageBufferManager object_property_manager{};
	void setupObjectAddress(const Scene& scene)
	{
		struct alignas(16) ObjectAddress
		{
			uint64_t vertex_address;
			uint64_t index_address;
		};

		struct alignas(16) ObjectProperty
		{
			Vector3f radiance{0};
			int is_light{0};
			int triangle_count{0};
			float area{0};
			float pad1{0};
			float pad2{0};
		};

		std::vector<ObjectAddress> all_object_address;
		std::vector<ObjectProperty> all_object_property;
		std::vector<int> luminous_indices{};

		uint64_t base_vertex_address = this->vertex_buffer_manager.getBufferAddress();
		uint64_t base_index_address = this->index_buffer_manager.getBufferAddress();
		auto& commands = this->indirect_buffer_manager.commands;
		for (size_t i = 0; i < commands.size(); i++)
		{
			auto object = scene.objects[i];
			auto& command = commands[i];
			uint64_t vertex_address = base_vertex_address + command.vertexOffset * sizeof(Vertex);
			uint64_t index_address = base_index_address + command.firstIndex * sizeof(Index);
			ObjectAddress address{vertex_address, index_address};

			if (object.is_light)
			{
				object.getArea();

				luminous_indices.push_back(i);
			}

			ObjectProperty property{object.radiance, object.is_light, object.triangle_count, object.area};
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

		this->object_luminous_indices_manager.setData(
			luminous_indices.data(), luminous_indices.size() * sizeof(int), luminous_indices.size());
		this->object_luminous_indices_manager.init();
	}

	void setupFunction();

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

	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;

	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;

	std::array<UniformBufferManager, MAX_FRAMES_IN_FLIGHT> uniform_buffer_managers{};

	std::array<DescriptorManager, MAX_FRAMES_IN_FLIGHT> descriptor_managers{};

	std::array<GeometryBufferManager, MAX_FRAMES_IN_FLIGHT> geometry_buffer_managers{};

	PipelineManager pipeline_manager{};

	VkStridedDeviceAddressRegionKHR ray_generate_region{};
	VkStridedDeviceAddressRegionKHR ray_miss_region{};
	VkStridedDeviceAddressRegionKHR ray_hit_region{};
	VkStridedDeviceAddressRegionKHR call_region{};

	std::array<StorageImageManager, 2> storage_image_managers{};

	StagingBufferManager sbt_buffer_manager{};

	std::vector<VkDescriptorImageInfo> result_images{};
	VkWriteDescriptorSetAccelerationStructureKHR write{};

	Matrix4f last_camera_matrix{1.0};
	Matrix4f current_camera_matrix{1.0};
};