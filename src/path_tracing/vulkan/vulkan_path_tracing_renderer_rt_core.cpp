#include <vulkan_path_tracing_renderer_rt_core.h>

VulkanPathTracingRendererRTCore::VulkanPathTracingRendererRTCore()
{
	this->context_manager.enable_ray_tracing = true;
}

VulkanPathTracingRendererRTCore::~VulkanPathTracingRendererRTCore()
{
	this->clear();
}

void VulkanPathTracingRendererRTCore::init()
{
	VulkanRendererBase::init(1);
	this->getFeatureProperty();
	this->setupFunction();
}

void VulkanPathTracingRendererRTCore::clear()
{
	VulkanRendererBase::clear();
}

void VulkanPathTracingRendererRTCore::setData(const Scene& scene)
{
	VulkanRendererBase::setData(scene);

	auto context_manager_sptr = std::make_shared<ContextManager>(this->context_manager);
	auto command_manager_sptr = std::make_shared<CommandManager>(this->command_manager);
	auto swap_chain_manager_sptr = std::make_shared<SwapChainManager>(this->swap_chain_manager);

	this->all_vertex_managers.resize(scene.objects.size());
	this->all_index_managers.resize(scene.objects.size());

	auto getTransformMatrix = [](const Matrix4f& model) {
		VkTransformMatrixKHR result{};
		for (uint32_t row = 0; row < 3; row++)
		{
			for (uint32_t col = 0; col < 4; col++)
			{
				result.matrix[row][col] = model[col][row];
			}
		}
		return result;
	};

	this->vertex_buffer_manager.clear();
	this->vertex_buffer_manager.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	this->vertex_buffer_manager.init();

	for (size_t i = 0; i < scene.objects.size(); i++)
	{
		auto& object = scene.objects[i];
		this->all_vertex_managers[i] = VertexBufferManager(context_manager_sptr, command_manager_sptr);
		this->all_vertex_managers[i].vertices = object.vertices;
		this->all_vertex_managers[i].usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
		this->all_vertex_managers[i].usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		this->all_vertex_managers[i].init();

		this->all_index_managers[i] = IndexBufferManager(context_manager_sptr, command_manager_sptr);
		this->all_index_managers[i].indices = object.indices;
		this->all_index_managers[i].enable_ray_tracing = true;
		this->all_index_managers[i].init();

		this->model_matrixes.push_back(getTransformMatrix(object.model));
	}

	this->all_blas_buffer_manager = StorageBufferManager(context_manager_sptr, command_manager_sptr);
	this->instance_buffer_manager = StorageBufferManager(context_manager_sptr, command_manager_sptr);
	this->tlas_buffer_manager = StorageBufferManager(context_manager_sptr, command_manager_sptr);
	this->scratch_buffer_manager = StorageBufferManager(context_manager_sptr, command_manager_sptr);
	this->sbt_buffer_manager = StagingBufferManager(context_manager_sptr, command_manager_sptr);
	this->object_address_manager = StorageBufferManager(context_manager_sptr, command_manager_sptr);
	this->object_property_manager = StorageBufferManager(context_manager_sptr, command_manager_sptr);
	this->object_luminous_indices_manager = StorageBufferManager(context_manager_sptr, command_manager_sptr);

	this->denoise_single_frame_image_manager = StorageImageManager(context_manager_sptr, command_manager_sptr);
	this->denoise_single_frame_image_manager.setExtent(this->swap_chain_manager.extent);
	this->denoise_single_frame_image_manager.init();

	this->gbuffer_position_image_manager = StorageImageManager(context_manager_sptr, command_manager_sptr);
	this->gbuffer_position_image_manager.setExtent(this->swap_chain_manager.extent);
	this->gbuffer_position_image_manager.init();

	this->gbuffer_normal_image_manager = StorageImageManager(context_manager_sptr, command_manager_sptr);
	this->gbuffer_normal_image_manager.setExtent(this->swap_chain_manager.extent);
	this->gbuffer_normal_image_manager.init();

	this->gbuffer_id_image_manager = StorageImageManager(context_manager_sptr, command_manager_sptr);
	this->gbuffer_id_image_manager.setExtent(this->swap_chain_manager.extent);
	this->gbuffer_id_image_manager.init();

	this->noise_image_manager = MultiStorageImageManager(context_manager_sptr, command_manager_sptr);
	this->noise_image_manager.setExtent(this->swap_chain_manager.extent);
	this->noise_image_manager.addImage(VK_FORMAT_R32G32B32A32_SFLOAT);
	this->noise_image_manager.addImage(VK_FORMAT_R32G32B32A32_SFLOAT);
	this->noise_image_manager.init();

	this->createBLAS();
	this->createTLAS();

	this->pipeline_manager = PipelineManager(context_manager_sptr, PipelineType::PathTracing);
	this->denoise_single_frame_pipeline_manager = PipelineManager(context_manager_sptr);
	this->denoise_time_accumulate_pipeline_manager = PipelineManager(context_manager_sptr);

	setupObjectAddress(scene);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		this->uniform_buffer_managers[i] = UniformBufferManager(context_manager_sptr, command_manager_sptr);
		this->uniform_buffer_managers[i].setData(&this->ubo, sizeof(UBOMVP), 1);
		this->uniform_buffer_managers[i].init();

		this->descriptor_managers[i] = DescriptorManager(context_manager_sptr);
		this->setupDescriptor(i);
		this->denoise_single_frame_descriptor_managers[i] = DescriptorManager(context_manager_sptr);
		this->setupDenoiseSingleFrameDescriptorSet(i);
		this->denoise_time_accumulate_descriptor_managers[i] = DescriptorManager(context_manager_sptr);
		this->setupDenoiseTimeAccumulateDescriptorSet(i);
	}

	this->render_pass_manager = RenderPassManager(context_manager_sptr, swap_chain_manager_sptr, command_manager_sptr);
	this->setupDenoisePostProcessingRenderPass();

	this->setupGraphicsPipelines();
	this->setupDenoiseSingleFramePipeline();
	this->setupDenoiseTimeAccumulatePipeline();

	this->createShaderBindingTable();

	setupImgui();
}

struct PushConstantRay
{
	Vector3f position;
	int image_index;

	Vector3f look;
	int frame;

	Vector3f up;
	int pad;
};

void VulkanPathTracingRendererRTCore::recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index)
{
	/* Clear Value */
	std::array<VkClearValue, 2> clear_values{};
	clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clear_values[1].depthStencil = {1.0f, 0};

	/* Dynamic setting of viewport */
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(this->swap_chain_manager.extent.width);
	viewport.height = static_cast<float>(this->swap_chain_manager.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	/* Dynamic setting of scissor rectangle */
	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = this->swap_chain_manager.extent;

	/* Push constants used by the denoising shader */
	DenoisePushConstantData denoise_push_constant{};
	denoise_push_constant.last_camera_matrix = this->last_camera_matrix;
	denoise_push_constant.current_index = this->current_frame;
	denoise_push_constant.frame_count = this->frame_count;

	VkCommandBufferBeginInfo command_begin{};
	command_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_begin.pNext = nullptr;
	command_begin.flags = 0;
	command_begin.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(command_buffer, &command_begin) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to begin recording command buffer!");
	}

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, this->pipeline_manager.pipeline);

	PushConstantRay temp{};
	temp.image_index = current_frame;

	if (this->moved)
	{
		this->moved = false;
		frame_count = 0;
	}
	temp.frame = frame_count++;
	temp.position = this->camera_position;
	vkCmdPushConstants(command_buffer,
					   this->pipeline_manager.layout,
					   VK_SHADER_STAGE_RAYGEN_BIT_KHR,
					   0,
					   sizeof(PushConstantRay),
					   &temp);

	vkCmdBindDescriptorSets(command_buffer,
							VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
							this->pipeline_manager.layout,
							0,
							1,
							&this->descriptor_managers[current_frame].set,
							0,
							nullptr);

	vkCmdTraceRaysKHR(command_buffer,
					  &this->ray_generate_region,
					  &this->ray_miss_region,
					  &this->ray_hit_region,
					  &this->call_region,
					  this->swap_chain_manager.extent.width,
					  this->swap_chain_manager.extent.height,
					  1);

	VkRenderPassBeginInfo render_pass_begin{};
	render_pass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin.renderPass = this->render_pass_manager.pass;
	render_pass_begin.framebuffer = this->render_pass_manager.buffers[image_index];
	render_pass_begin.renderArea.offset = {0, 0};
	render_pass_begin.renderArea.extent = this->swap_chain_manager.extent;
	render_pass_begin.clearValueCount = static_cast<uint32_t>(clear_values.size());
	render_pass_begin.pClearValues = clear_values.data();

	vkCmdBeginRenderPass(command_buffer, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

	/* ========== Joint bilateral filtering ========== */

	vkCmdBindPipeline(
		command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->denoise_single_frame_pipeline_manager.pipeline);

	vkCmdSetViewport(command_buffer, 0, 1, &viewport);

	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	vkCmdBindDescriptorSets(command_buffer,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							this->denoise_single_frame_pipeline_manager.layout,
							0,
							1,
							&this->denoise_single_frame_descriptor_managers[current_frame].set,
							0,
							nullptr);

	vkCmdPushConstants(command_buffer,
					   this->denoise_single_frame_pipeline_manager.layout,
					   VK_SHADER_STAGE_FRAGMENT_BIT,
					   0,
					   sizeof(DenoisePushConstantData),
					   &denoise_push_constant);

	vkCmdDraw(command_buffer, 6, 1, 0, 0);

	/* ========== Denoise time accumulate subpass ========== */
	vkCmdNextSubpass(command_buffer, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(
		command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->denoise_time_accumulate_pipeline_manager.pipeline);

	vkCmdSetViewport(command_buffer, 0, 1, &viewport);

	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	vkCmdBindDescriptorSets(command_buffer,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							this->denoise_time_accumulate_pipeline_manager.layout,
							0,
							1,
							&this->denoise_time_accumulate_descriptor_managers[current_frame].set,
							0,
							nullptr);

	vkCmdPushConstants(command_buffer,
					   this->denoise_time_accumulate_pipeline_manager.layout,
					   VK_SHADER_STAGE_FRAGMENT_BIT,
					   0,
					   sizeof(DenoisePushConstantData),
					   &denoise_push_constant);

	vkCmdDraw(command_buffer, 6, 1, 0, 0);

	updateImgui(command_buffer);

	vkCmdEndRenderPass(command_buffer);

	if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record command buffer!");
	}
}

void VulkanPathTracingRendererRTCore::setupGraphicsPipelines()
{
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups(4);
	for (auto& group : groups)
	{
		group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		group.pNext = nullptr;
		group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		group.generalShader = VK_SHADER_UNUSED_KHR;
		group.closestHitShader = VK_SHADER_UNUSED_KHR;
		group.anyHitShader = VK_SHADER_UNUSED_KHR;
		group.intersectionShader = VK_SHADER_UNUSED_KHR;
		group.pShaderGroupCaptureReplayHandle = nullptr;
	}

	this->pipeline_manager.addShaderStage("path_tracing_rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR);
	groups[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	groups[0].generalShader = 0;

	this->pipeline_manager.addShaderStage("path_tracing_rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR);
	groups[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
	groups[1].generalShader = 1;

	this->pipeline_manager.addShaderStage("path_tracing_rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	groups[2].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	groups[2].closestHitShader = 2;

	this->pipeline_manager.addShaderStage("path_tracing_shadow_rahit.spv", VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
	this->pipeline_manager.addShaderStage("path_tracing_shadow_rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
	groups[3].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	groups[3].anyHitShader = 3;
	groups[3].closestHitShader = 4;

	this->pipeline_manager.setShaderGroups(groups);

	std::vector<VkDescriptorSetLayout> descriptor = {this->descriptor_managers[0].layout};
	std::vector<VkPushConstantRange> push(1);
	push[0].offset = 0;
	push[0].size = sizeof(PushConstantRay);
	push[0].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

	this->pipeline_manager.setLayout(descriptor, push);

	this->pipeline_manager.init();
}

void VulkanPathTracingRendererRTCore::getFeatureProperty()
{
	this->ray_tracing_property.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	this->acceleration_property.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
	this->ray_tracing_property.pNext = &this->acceleration_property;

	VkPhysicalDeviceProperties2 device_properties{};
	device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	device_properties.pNext = &this->ray_tracing_property;
	vkGetPhysicalDeviceProperties2(this->context_manager.physical_device, &device_properties);

	this->ray_tracing_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	this->acceleration_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	this->ray_tracing_feature.pNext = &this->acceleration_feature;

	VkPhysicalDeviceFeatures2 device_feature{};
	device_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	device_feature.pNext = &this->ray_tracing_feature;
	vkGetPhysicalDeviceFeatures2(this->context_manager.physical_device, &device_feature);
}

void VulkanPathTracingRendererRTCore::createBLAS()
{
	size_t geometry_numbers = this->all_vertex_managers.size();

	std::vector<VkAccelerationStructureGeometryTrianglesDataKHR> triangle_datas{};
	std::vector<VkAccelerationStructureGeometryKHR> geometries{};
	std::vector<VkAccelerationStructureBuildGeometryInfoKHR> build_geometries{};
	std::vector<VkAccelerationStructureBuildRangeInfoKHR> build_ranges{};
	std::vector<VkAccelerationStructureCreateInfoKHR> acceleration_creates{};

	triangle_datas.resize(geometry_numbers);
	geometries.resize(geometry_numbers);
	build_geometries.resize(geometry_numbers);
	build_ranges.resize(geometry_numbers);
	acceleration_creates.resize(geometry_numbers);

	this->blas.resize(geometry_numbers);

	VkDeviceSize all_size = 0;
	VkDeviceSize min_build_size = 0;
	for (size_t i = 0; i < geometry_numbers; i++)
	{
		triangle_datas[i].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		triangle_datas[i].pNext = nullptr;
		triangle_datas[i].vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		triangle_datas[i].vertexData.deviceAddress = this->all_vertex_managers[i].getBufferAddress();
		triangle_datas[i].vertexStride = sizeof(Vertex);
		triangle_datas[i].maxVertex = this->all_vertex_managers[i].vertices.size() - 1;
		triangle_datas[i].indexType = VK_INDEX_TYPE_UINT32;
		triangle_datas[i].indexData.deviceAddress = this->all_index_managers[i].getBufferAddress();
		triangle_datas[i].transformData.deviceAddress = 0;

		geometries[i].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometries[i].pNext = nullptr;
		geometries[i].geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		geometries[i].geometry.triangles = triangle_datas[i];
		geometries[i].flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

		build_ranges[i].primitiveCount = this->all_index_managers[i].indices.size() / 3;
		build_ranges[i].primitiveOffset = 0;
		build_ranges[i].firstVertex = 0;
		build_ranges[i].transformOffset = 0;

		build_geometries[i].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		build_geometries[i].pNext = nullptr;
		build_geometries[i].type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		build_geometries[i].flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		build_geometries[i].mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		build_geometries[i].srcAccelerationStructure = VK_NULL_HANDLE;
		build_geometries[i].dstAccelerationStructure = VK_NULL_HANDLE;
		build_geometries[i].geometryCount = 1;
		build_geometries[i].pGeometries = &geometries[i];
		build_geometries[i].ppGeometries = nullptr;
		build_geometries[i].scratchData;

		VkAccelerationStructureBuildSizesInfoKHR build_size{};
		build_size.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		build_size.pNext = nullptr;
		uint32_t counts[] = {this->all_index_managers[i].indices.size() / 3};
		vkGetAccelerationStructureBuildSizesKHR(this->context_manager.device,
												VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
												&build_geometries[i],
												counts,
												&build_size);

		acceleration_creates[i].sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		acceleration_creates[i].pNext = nullptr;
		acceleration_creates[i].createFlags = 0;
		acceleration_creates[i].buffer = VK_NULL_HANDLE;
		acceleration_creates[i].offset = all_size;
		acceleration_creates[i].size = build_size.accelerationStructureSize;
		acceleration_creates[i].type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		acceleration_creates[i].deviceAddress = 0;

		all_size += build_size.accelerationStructureSize;
		all_size = align(all_size, 256);

		min_build_size = std::max(min_build_size, build_size.buildScratchSize);
	}

	std::vector<char> temp(all_size, 0);
	this->all_blas_buffer_manager.setData(temp.data(), all_size, this->all_vertex_managers.size());
	this->all_blas_buffer_manager.enable_ray_tracing = true;
	this->all_blas_buffer_manager.init();

	temp.clear();
	temp.resize(min_build_size, 0);
	this->scratch_buffer_manager.setData(temp.data(), min_build_size, 1);
	this->scratch_buffer_manager.enable_ray_tracing = true;
	this->scratch_buffer_manager.init();

	for (size_t i = 0; i < geometry_numbers; i++)
	{
		acceleration_creates[i].buffer = this->all_blas_buffer_manager.buffer;
		vkCreateAccelerationStructureKHR(
			this->context_manager.device, &acceleration_creates[i], nullptr, &this->blas[i]);
	}

	for (size_t i = 0; i < geometry_numbers; i++)
	{
		build_geometries[i].dstAccelerationStructure = this->blas[i];
		build_geometries[i].scratchData.deviceAddress = this->scratch_buffer_manager.getBufferAddress();
		VkAccelerationStructureBuildRangeInfoKHR* ranges[] = {&build_ranges[i]};

		auto command_buffer = this->command_manager.beginGraphicsCommands();

		vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &build_geometries[i], ranges);

		this->command_manager.endGraphicsCommands(command_buffer);
	}

	this->scratch_buffer_manager.clear();
}

void VulkanPathTracingRendererRTCore::createTLAS()
{
	std::vector<VkAccelerationStructureInstanceKHR> instances{};
	instances.resize(this->blas.size());
	for (size_t i = 0; i < this->blas.size(); i++)
	{
		VkAccelerationStructureDeviceAddressInfoKHR address{};
		address.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		address.accelerationStructure = this->blas[i];

		instances[i].transform = this->model_matrixes[i];
		instances[i].instanceCustomIndex = static_cast<uint32_t>(i);
		instances[i].mask = 0xFF;
		instances[i].instanceShaderBindingTableRecordOffset = 0;
		instances[i].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		instances[i].accelerationStructureReference =
			vkGetAccelerationStructureDeviceAddressKHR(this->context_manager.device, &address);
	}

	this->instance_buffer_manager.setData(
		instances.data(), instances.size() * sizeof(VkAccelerationStructureInstanceKHR), instances.size());
	this->instance_buffer_manager.enable_ray_tracing = true;
	this->instance_buffer_manager.init();

	VkAccelerationStructureGeometryInstancesDataKHR geometry_instances{};
	geometry_instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	geometry_instances.pNext = nullptr;
	geometry_instances.arrayOfPointers = VK_FALSE;
	geometry_instances.data.deviceAddress = this->instance_buffer_manager.getBufferAddress();

	VkAccelerationStructureGeometryKHR geometry{};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.pNext = nullptr;
	geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	geometry.geometry.instances = geometry_instances;
	geometry.flags = 0;

	VkAccelerationStructureBuildGeometryInfoKHR build_geometry{};
	build_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	build_geometry.pNext = nullptr;
	build_geometry.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	build_geometry.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	build_geometry.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	build_geometry.srcAccelerationStructure = VK_NULL_HANDLE;
	build_geometry.dstAccelerationStructure = VK_NULL_HANDLE;
	build_geometry.geometryCount = 1;
	build_geometry.pGeometries = &geometry;
	build_geometry.ppGeometries = nullptr;
	build_geometry.scratchData;

	VkAccelerationStructureBuildSizesInfoKHR build_size{};
	build_size.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	build_size.pNext = nullptr;
	uint32_t counts[] = {this->blas.size()};
	vkGetAccelerationStructureBuildSizesKHR(this->context_manager.device,
											VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
											&build_geometry,
											counts,
											&build_size);

	std::vector<char> temp(build_size.accelerationStructureSize, 0);
	this->tlas_buffer_manager.setData(temp.data(), build_size.accelerationStructureSize, 1);
	this->tlas_buffer_manager.enable_ray_tracing = true;
	this->tlas_buffer_manager.init();

	temp.clear();
	temp.resize(build_size.buildScratchSize, 0);
	this->scratch_buffer_manager.setData(temp.data(), build_size.buildScratchSize, 1);
	this->scratch_buffer_manager.enable_ray_tracing = true;
	this->scratch_buffer_manager.init();

	VkAccelerationStructureCreateInfoKHR acceleration_create{};
	acceleration_create.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	acceleration_create.pNext = nullptr;
	acceleration_create.createFlags = 0;
	acceleration_create.buffer = this->tlas_buffer_manager.buffer;
	acceleration_create.offset = 0;
	acceleration_create.size = build_size.accelerationStructureSize;
	acceleration_create.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	acceleration_create.deviceAddress = 0;
	vkCreateAccelerationStructureKHR(this->context_manager.device, &acceleration_create, nullptr, &this->tlas);

	build_geometry.dstAccelerationStructure = this->tlas;
	build_geometry.scratchData.deviceAddress = this->scratch_buffer_manager.getBufferAddress();

	VkAccelerationStructureBuildRangeInfoKHR build_range{this->blas.size(), 0, 0, 0};
	VkAccelerationStructureBuildRangeInfoKHR* ranges[] = {&build_range};

	auto command_buffer = this->command_manager.beginGraphicsCommands();

	vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &build_geometry, ranges);

	this->command_manager.endGraphicsCommands(command_buffer);

	this->scratch_buffer_manager.clear();
}

void VulkanPathTracingRendererRTCore::createShaderBindingTable()
{
	/* Number of shader groups */
	uint32_t ray_gen_shader_count = 1; // ray generation
	uint32_t miss_shader_count = 1;	   // miss shader
	uint32_t hit_shader_count = 2;	   // 2 hit groups: main hit + shadow hit

	uint32_t handle_count = ray_gen_shader_count + miss_shader_count + hit_shader_count;

	uint32_t handle_size = this->ray_tracing_property.shaderGroupHandleSize;
	uint32_t alignment = this->ray_tracing_property.shaderGroupHandleAlignment;
	uint32_t handle_size_aligned = align(handle_size, alignment);

	// Ray Generation Region
	this->ray_generate_region.stride = align(handle_size_aligned, this->ray_tracing_property.shaderGroupBaseAlignment);
	this->ray_generate_region.size = this->ray_generate_region.stride;

	// Miss Region
	this->ray_miss_region.stride = handle_size_aligned;
	this->ray_miss_region.size =
		align(miss_shader_count * handle_size_aligned, this->ray_tracing_property.shaderGroupBaseAlignment);

	// Hit Region
	this->ray_hit_region.stride = handle_size_aligned;
	this->ray_hit_region.size =
		align(hit_shader_count * handle_size_aligned, this->ray_tracing_property.shaderGroupBaseAlignment);

	// 获取所有着色器组的句柄
	std::vector<uint8_t> handles(handle_count * handle_size);
	vkGetRayTracingShaderGroupHandlesKHR(this->context_manager.device,
										 this->pipeline_manager.pipeline,
										 0,			   // 第一个组
										 handle_count, // 组数量
										 static_cast<uint32_t>(handles.size()),
										 handles.data());

	// 计算 SBT 总大小
	VkDeviceSize sbt_size = ray_generate_region.size + ray_miss_region.size + ray_hit_region.size;

	// 初始化 SBT Buffer
	this->sbt_buffer_manager.usage =
		VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	this->sbt_buffer_manager.size = sbt_size;
	this->sbt_buffer_manager.init();

	/* Set the device address of each SBT area */
	VkDeviceAddress sbt_address = this->sbt_buffer_manager.getBufferAddress();
	this->ray_generate_region.deviceAddress = sbt_address;
	this->ray_miss_region.deviceAddress = sbt_address + this->ray_generate_region.size;
	this->ray_hit_region.deviceAddress = sbt_address + this->ray_generate_region.size + this->ray_miss_region.size;

	// 复制句柄到 SBT Buffer
	uint8_t* sbt_data = reinterpret_cast<uint8_t*>(this->sbt_buffer_manager.mapped);

	// 1. Ray Generation Shader (index 0)
	memcpy(sbt_data, handles.data(), handle_size);

	// 2. Miss Shader (index 1)
	memcpy(sbt_data + ray_generate_region.size, handles.data() + handle_size, handle_size);

	// 3. Hit Groups (index 2 and 3)
	// - Default Hit Group (closest hit)
	memcpy(sbt_data + ray_generate_region.size + ray_miss_region.size, handles.data() + 2 * handle_size, handle_size);
	// - Shadow Hit Group (any hit + closest hit)
	memcpy(sbt_data + ray_generate_region.size + ray_miss_region.size + handle_size_aligned,
		   handles.data() + 3 * handle_size,
		   handle_size);
}

void VulkanPathTracingRendererRTCore::setupDescriptor(const int index)
{
	VkDescriptorSetLayoutBinding layout_binding{};
	/* ========== Layout binding infomation ========== */
	/* TLAS binding */
	layout_binding.binding = 0;
	layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	layout_binding.descriptorCount = 1;
	layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	layout_binding.pImmutableSamplers = nullptr;
	this->descriptor_managers[index].addLayoutBinding(layout_binding);

	/* Noisy color result */
	this->descriptor_managers[index].addDescriptor(
		this->noise_image_manager.getDescriptor(1, VK_SHADER_STAGE_RAYGEN_BIT_KHR));

	/* Camera information descriptor */
	this->descriptor_managers[index].addLayoutBinding(
		this->uniform_buffer_managers[index].getLayoutBinding(2, VK_SHADER_STAGE_RAYGEN_BIT_KHR));

	/* Texture sampler descriptor */
	this->descriptor_managers[index].addDescriptor(
		this->texture_manager.getDescriptor(3, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));

	/* Object's vertex and index address descriptor */
	this->descriptor_managers[index].addDescriptor(
		this->object_address_manager.getDescriptor(4, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));

	/* Object's property descriptor */
	this->descriptor_managers[index].addDescriptor(
		this->object_property_manager.getDescriptor(5, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));

	/* Material index descriptor */
	this->descriptor_managers[index].addDescriptor(
		this->material_index_manager.getDescriptor(6, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));

	/* Material data descriptor */
	this->descriptor_managers[index].addDescriptor(
		this->material_ssbo_manager.getDescriptor(7, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));

	/* Luminous object index descriptor */
	this->descriptor_managers[index].addDescriptor(
		this->object_luminous_indices_manager.getDescriptor(8, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));

	this->descriptor_managers[index].addDescriptor(this->gbuffer_position_image_manager.getDescriptor(
		9, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR));

	this->descriptor_managers[index].addDescriptor(this->gbuffer_normal_image_manager.getDescriptor(
		10, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR));

	this->descriptor_managers[index].addDescriptor(this->gbuffer_id_image_manager.getDescriptor(
		11, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR));

	/* ========== Write Descriptor Set ========== */
	/* TLAS write */
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	write.pNext = nullptr;
	write.accelerationStructureCount = 1;
	write.pAccelerationStructures = &tlas;
	VkWriteDescriptorSet tlas_write{};
	tlas_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	tlas_write.pNext = &write;
	tlas_write.dstSet = VK_NULL_HANDLE;
	tlas_write.dstBinding = 0;
	tlas_write.dstArrayElement = 0;
	tlas_write.descriptorCount = 1;
	tlas_write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	tlas_write.pBufferInfo = nullptr;
	tlas_write.pImageInfo = nullptr;
	tlas_write.pTexelBufferView = nullptr;
	this->descriptor_managers[index].addWrite(tlas_write);

	/* Camera set write information */
	this->descriptor_managers[index].addWrite(this->uniform_buffer_managers[index].getWriteInformation(2));

	this->descriptor_managers[index].init();
}

void VulkanPathTracingRendererRTCore::setupFunction()
{
	this->vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(
		this->context_manager.device, "vkCmdBuildAccelerationStructuresKHR");

	this->vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(
		this->context_manager.device, "vkGetRayTracingShaderGroupHandlesKHR");

	this->vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(
		this->context_manager.device, "vkGetAccelerationStructureBuildSizesKHR");

	this->vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(
		this->context_manager.device, "vkCreateAccelerationStructureKHR");

	this->vkGetAccelerationStructureDeviceAddressKHR =
		(PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(
			this->context_manager.device, "vkGetAccelerationStructureDeviceAddressKHR");

	this->vkCmdTraceRaysKHR =
		(PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(this->context_manager.device, "vkCmdTraceRaysKHR");
}

void VulkanPathTracingRendererRTCore::createAcceleration()
{
	VkAccelerationStructureCreateInfoKHR;
	VkAccelerationStructureGeometryKHR;
}