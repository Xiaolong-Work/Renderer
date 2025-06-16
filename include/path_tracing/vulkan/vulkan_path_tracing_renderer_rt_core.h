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

#include <NRD.h>

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

	void updateUniformBufferObjects(const int index) override;

	void setupGraphicsPipelines();

	void setupDenoiseSingleFrameSubpass();

	void setupDenoiseSingleFrameDescriptorSet(const int index);

	void setupDenoiseSingleFramePipeline();

	void setupDenoiseTimeAccumulateSubpass();

	void setupDenoiseTimeAccumulateDescriptorSet(const int index);

	void setupDenoiseTimeAccumulatePipeline();

	void setupDenoisePostProcessingRenderPass();

	void setupObjectAddress(const Scene& scene);

	void setupFunction();

	void updateImgui(VkCommandBuffer command_buffer);

protected:
	void getFeatureProperty();

	void createAcceleration();

	void createBLAS();

	void createTLAS();

	void setupDescriptor(const int index);

	void createShaderBindingTable();

	void setupNRD()
	{
		nrd::InstanceCreationDesc instance_creation{};
	}

	void denoNRD()
	{
		nrd::CommonSettings settings{};
	}

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

	/* Extension Functions */
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;

	std::array<UniformBufferManager, MAX_FRAMES_IN_FLIGHT> uniform_buffer_managers{};

	std::array<DescriptorManager, MAX_FRAMES_IN_FLIGHT> descriptor_managers{};

	std::array<DescriptorManager, MAX_FRAMES_IN_FLIGHT> denoise_single_frame_descriptor_managers{};

	std::array<DescriptorManager, MAX_FRAMES_IN_FLIGHT> denoise_time_accumulate_descriptor_managers{};

	PipelineManager pipeline_manager{};

	VkStridedDeviceAddressRegionKHR ray_generate_region{};
	VkStridedDeviceAddressRegionKHR ray_miss_region{};
	VkStridedDeviceAddressRegionKHR ray_hit_region{};
	VkStridedDeviceAddressRegionKHR call_region{};

	MultiStorageImageManager noise_image_manager{};

	StagingBufferManager sbt_buffer_manager{};

	VkWriteDescriptorSetAccelerationStructureKHR write{};

	Matrix4f last_camera_matrix{1.0};
	Matrix4f current_camera_matrix{1.0};

	/* Single frame noise reduction */
	PipelineManager denoise_single_frame_pipeline_manager{};
	PipelineManager denoise_time_accumulate_pipeline_manager{};

	StorageImageManager denoise_single_frame_image_manager{};

	StorageImageManager gbuffer_position_image_manager{};
	StorageImageManager gbuffer_normal_image_manager{};
	StorageImageManager gbuffer_id_image_manager{};

	static const int NUM_SAMPLES = 100;
	float frame_times[NUM_SAMPLES] = {};
	int frame_index = 0;

	RandomBufferManager random_buffer_manager{};

	struct DenoiseParam
	{
		Matrix4f last_camera_matrix{1};
		int current_index{0};
		int frame_count{0};
		int kernel_size{8};
		float sigma_point{32.0};
		float sigma_color{0.6};
		float sigma_normal{0.1};
		float sigma_plane{0.1};
		bool enable_single_frame_denoise{true};
	} denoise_param;

	struct RayTracingParam
	{
		int spp{1};
		int max_depth{5};
	};

	struct PushConstantRay
	{
		Vector3f position;
		int image_index;

		Vector3f look;
		int frame;

		Vector3f up;
		int pad;
	};

	StorageBufferManager object_luminous_indices_manager{};
	StorageBufferManager object_address_manager{};
	StorageBufferManager object_property_manager{};
};