#include <vulkan_path_tracing_renderer_rt_core.h>

VulkanPathTracingRendererRTCore::VulkanPathTracingRendererRTCore()
{
	this->context_manager.enable_ray_tracing = true;
	this->init();
}

VulkanPathTracingRendererRTCore::~VulkanPathTracingRendererRTCore()
{
	this->clear();
}

void VulkanPathTracingRendererRTCore::init()
{
	VulkanRendererBase::init(1);
	this->getFeatureProperty();

	this->vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(
		this->context_manager.device, "vkCmdBuildAccelerationStructuresKHR");
	this->vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(
		this->context_manager.device, "vkCreateAccelerationStructureKHR");

	this->vkCmdTraceRaysKHR =
		(PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(this->context_manager.device, "vkCmdTraceRaysKHR");

	this->vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(
		this->context_manager.device, "vkGetRayTracingShaderGroupHandlesKHR");
}

void VulkanPathTracingRendererRTCore::clear()
{
}

void VulkanPathTracingRendererRTCore::setData(const Scene& scene)
{
	VulkanRendererBase::setData(scene);

	auto context_manager_sptr = std::make_shared<ContextManager>(this->context_manager);
	auto command_manager_sptr = std::make_shared<CommandManager>(this->command_manager);

	this->all_vertex_managers.resize(scene.objects.size());
	this->all_index_managers.resize(scene.objects.size());

	auto getTransformMatrix = [](const Matrix4f& model) {
		VkTransformMatrixKHR result;

		for (uint32_t row = 0; row < 3; row++)
		{
			for (uint32_t col = 0; col < 4; col++)
			{
				result.matrix[row][col] = model[col][row];
			}
		}

		return result;
	};

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

	for (int i = 0; i < 2; i++)
	{
		this->storage_image_managers[i] = StorageImageManager(context_manager_sptr, command_manager_sptr);
		this->storage_image_managers[i].setExtent(this->swap_chain_manager.extent);
		this->storage_image_managers[i].init();
	}

	this->createBLAS();
	this->createTLAS();

	this->pipeline_manager = PipelineManager(context_manager_sptr, PipelineType::PathTracing);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		this->uniform_buffer_managers[i] = UniformBufferManager(context_manager_sptr, command_manager_sptr);
		this->uniform_buffer_managers[i].setData(&this->ubo, sizeof(UBOMVP), 1);
		this->uniform_buffer_managers[i].init();
		this->descriptor_managers[i] = DescriptorManager(context_manager_sptr);
		this->setupDescriptor(i);
	}

	this->setupGraphicsPipelines();

	createShaderBindingTable();
}

struct PushConstantRay
{
	glm::vec4 clearColor = glm::vec4(0, 0, 0, 1);
	glm::vec3 lightPosition;
	float lightIntensity;
	int lightType;
	int frame;
};

void VulkanPathTracingRendererRTCore::recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index)
{
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
	temp.frame = 0;
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

	if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record command buffer!");
	}
}



void VulkanPathTracingRendererRTCore::setupGraphicsPipelines()
{
	this->pipeline_manager.addShaderStage("path_tracing_rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR);
	this->pipeline_manager.addShaderStage("path_tracing_rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR);
	this->pipeline_manager.addShaderStage("path_tracing_rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

	

	std::vector<VkPushConstantRange> push(1);
	push[0].offset = 0;
	push[0].size = sizeof(PushConstantRay);
	push[0].stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

	std::vector<VkDescriptorSetLayout> layout = {this->descriptor_managers[0].layout};
	this->pipeline_manager.setDescriptorSetLayout(layout, push);

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
	auto& commands = this->indirect_buffer_manager.commands;

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
		auto vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(
			this->context_manager.device, "vkGetAccelerationStructureBuildSizesKHR");

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

		if (all_size % 256 != 0)
		{
			all_size = all_size - (all_size % 256) + 256;
		}

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
		auto vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(
			this->context_manager.device, "vkCreateAccelerationStructureKHR");
		vkCreateAccelerationStructureKHR(
			this->context_manager.device, &acceleration_creates[i], nullptr, &this->blas[i]);
	}

	for (size_t i = 0; i < geometry_numbers; i++)
	{
		build_geometries[i].dstAccelerationStructure = this->blas[i];
		build_geometries[i].scratchData.deviceAddress = this->scratch_buffer_manager.getBufferAddress();

		VkAccelerationStructureBuildRangeInfoKHR* temp_range[] = {&build_ranges[i]};
		auto command_buffer = this->command_manager.beginGraphicsCommands();

		auto vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(
			this->context_manager.device, "vkCmdBuildAccelerationStructuresKHR");

		vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &build_geometries[i], temp_range);
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
		instances[i].transform = this->model_matrixes[i];
		instances[i].instanceCustomIndex = static_cast<uint32_t>(i);
		instances[i].mask = 0xFF;
		instances[i].instanceShaderBindingTableRecordOffset = 0;
		instances[i].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;

		VkAccelerationStructureDeviceAddressInfoKHR address{};
		address.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		address.accelerationStructure = this->blas[i];
		auto vkGetAccelerationStructureDeviceAddressKHR =
			(PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(
				this->context_manager.device, "vkGetAccelerationStructureDeviceAddressKHR");
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
	auto vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(
		this->context_manager.device, "vkGetAccelerationStructureBuildSizesKHR");

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
	VkAccelerationStructureBuildRangeInfoKHR* temp_range[] = {&build_range};

	auto command_buffer = this->command_manager.beginGraphicsCommands();
	vkCmdBuildAccelerationStructuresKHR(command_buffer, 1, &build_geometry, temp_range);
	this->command_manager.endGraphicsCommands(command_buffer);

	this->scratch_buffer_manager.clear();
}

void VulkanPathTracingRendererRTCore::createShaderBindingTable()
{
	uint32_t miss_shader_count{1};
	uint32_t hit_shader_count{1};

	uint32_t handle_count = 1 + miss_shader_count + hit_shader_count; // ray generate + miss hit + close hit

	uint32_t handle_size = this->ray_tracing_property.shaderGroupHandleSize;
	uint32_t alignment = this->ray_tracing_property.shaderGroupHandleAlignment;

	uint32_t handle_size_aligned = align(handle_size, alignment);

	this->ray_generate_region.stride = align(handle_size_aligned, this->ray_tracing_property.shaderGroupBaseAlignment);
	this->ray_generate_region.size = this->ray_generate_region.stride;

	this->ray_miss_region.stride = handle_size_aligned;
	this->ray_miss_region.size =
		align(miss_shader_count * handle_size_aligned, this->ray_tracing_property.shaderGroupBaseAlignment);

	this->ray_hit_region.stride = handle_size_aligned;
	this->ray_hit_region.size =
		align(hit_shader_count * handle_size_aligned, this->ray_tracing_property.shaderGroupBaseAlignment);

	std::vector<uint8_t> handles(handle_count * handle_size);
	vkGetRayTracingShaderGroupHandlesKHR(this->context_manager.device,
										 this->pipeline_manager.pipeline,
										 0,
										 handle_count,
										 static_cast<uint32_t>(handles.size()),
										 handles.data());

	VkDeviceSize sbt_size = ray_generate_region.size + ray_miss_region.size + ray_hit_region.size;

	this->sbt_buffer_manager.usage =
		VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	this->sbt_buffer_manager.size = sbt_size;
	this->sbt_buffer_manager.init();

	VkDeviceAddress sbt_address = this->sbt_buffer_manager.getBufferAddress();
	this->ray_generate_region.deviceAddress = sbt_address;
	this->ray_miss_region.deviceAddress = sbt_address + this->ray_generate_region.size;
	this->ray_hit_region.deviceAddress = sbt_address + this->ray_generate_region.size + this->ray_miss_region.size;

	uint8_t* sbt_data = reinterpret_cast<uint8_t*>(this->sbt_buffer_manager.mapped);
	memcpy(sbt_data, handles.data(), handle_size);												  // 复制光线生成句柄
	memcpy(sbt_data + this->ray_generate_region.size, handles.data() + handle_size, handle_size); // 复制未命中句柄
	memcpy(sbt_data + this->ray_generate_region.size + this->ray_miss_region.size,
		   handles.data() + 2 * handle_size,
		   handle_size); // 复制命中句柄
}

void VulkanPathTracingRendererRTCore::setupDescriptor(const int index)
{
	VkDescriptorSetLayoutBinding layout_binding{};
	/* ========== Layout binding infomation ========== */
	/* TLAS binding */
	layout_binding.binding = 0;
	layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	layout_binding.descriptorCount = 1;
	layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	layout_binding.pImmutableSamplers = nullptr;
	this->descriptor_managers[index].addLayoutBinding(layout_binding);

	/* Result image binding */
	layout_binding.binding = 1;
	layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	layout_binding.descriptorCount = 2;
	layout_binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	layout_binding.pImmutableSamplers = nullptr;
	this->descriptor_managers[index].addLayoutBinding(layout_binding);

	/* Camera information binding */
	this->descriptor_managers[index].addLayoutBinding(
		this->uniform_buffer_managers[index].getLayoutBinding(2, VK_SHADER_STAGE_RAYGEN_BIT_KHR));

	/* ========== Pool size infomation ========== */
	/* TLAS pool size */
	this->descriptor_managers[index].addPoolSize(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1);

	/* Result image pool size */
	this->descriptor_managers[index].addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1);

	/* Camera pool size */
	this->descriptor_managers[index].addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);

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
	result_image_write.dstBinding = 1;
	result_image_write.dstArrayElement = 0;
	result_image_write.descriptorCount = static_cast<uint32_t>(result_images.size());
	result_image_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	result_image_write.pBufferInfo = nullptr;
	result_image_write.pImageInfo = result_images.data();
	result_image_write.pTexelBufferView = nullptr;
	this->descriptor_managers[index].addWrite(result_image_write);

	/* Camera set write information */
	this->descriptor_managers[index].addWrite(this->uniform_buffer_managers[index].getWriteInformation(2));

	this->descriptor_managers[index].init();
}

void VulkanPathTracingRendererRTCore::createAcceleration()
{
	VkAccelerationStructureCreateInfoKHR;
	VkAccelerationStructureGeometryKHR;
}