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

	this->createBLAS();
	this->createTLAS();

	this->pipeline_manager = PipelineManager(context_manager_sptr, PipelineType::PathTracing);
}

void VulkanPathTracingRendererRTCore::setupGraphicsPipelines()
{
	std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	this->pipeline_manager.dynamic_states = dynamicStates;

	this->pipeline_manager.addShaderStage("path_tracing_rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR);
	this->pipeline_manager.addShaderStage("path_tracing_rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR);
	this->pipeline_manager.addShaderStage("path_tracing_rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

	std::vector<VkDescriptorSetLayout> layout = {this->descriptor_managers[0].layout};
	this->pipeline_manager.setDescriptorSetLayout(layout);

	this->pipeline_manager.init();

	vkGetRayTracingShaderGroupHandlesKHR
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

void VulkanPathTracingRendererRTCore::setupDescriptor(const int index)
{
	/* ========== Layout binding infomation ========== */
	/* MVP UBO binding */
	this->descriptor_managers[index].addLayoutBinding(
		this->uniform_buffer_managers[index].getLayoutBinding(0, VK_SHADER_STAGE_VERTEX_BIT));

	/* Texture sampler binding */
	this->descriptor_managers[index].addLayoutBinding(
		this->texture_manager.getLayoutBinding(2, VK_SHADER_STAGE_FRAGMENT_BIT));

	VkWriteDescriptorSetAccelerationStructureKHR write{}; 
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	write.pNext = nullptr;
	write.accelerationStructureCount = 1;
	write.pAccelerationStructures = &tlas;


	/* Material data binding */
	this->descriptor_managers[index].addLayoutBinding(
		this->material_ssbo_manager.getLayoutBinding(6, VK_SHADER_STAGE_FRAGMENT_BIT));

	/* ========== Pool size infomation ========== */
	/* UBO pool size */
	this->descriptor_managers[index].addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);

	/* Sampler pool size */
	this->descriptor_managers[index].addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2);

	/* SSBO pool size */
	this->descriptor_managers[index].addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5);

	/* ========== Write Descriptor Set ========== */
	std::vector<VkWriteDescriptorSet> descriptor_writes{};

	/* MVP UBO set write information */
	this->descriptor_managers[index].addWrite(this->uniform_buffer_managers[index].getWriteInformation(0));

	this->descriptor_managers[index].addWrite(this->model_matrix_manager.getWriteInformation(1));

	/* Texture sampler write information */
	if (!this->texture_manager.isEmpty())
	{
		this->descriptor_managers[index].addWrite(this->texture_manager.getWriteInformation(2));
	}



	/* Material data set write information */
	this->descriptor_managers[index].addWrite(this->material_ssbo_manager.getWriteInformation(6));

	this->descriptor_managers[index].addWrite(this->shadow_map_manager.mvp_ssbo_manager.getWriteInformation(7));

	this->descriptor_managers[index].init();
}

void VulkanPathTracingRendererRTCore::createAcceleration()
{
	VkAccelerationStructureCreateInfoKHR;
	VkAccelerationStructureGeometryKHR;
}