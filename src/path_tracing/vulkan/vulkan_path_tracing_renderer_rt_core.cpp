#include <vulkan_path_tracing_renderer_rt_core.h>

VulkanPathTracingRendererRTCore::VulkanPathTracingRendererRTCore()
{
	this->init();
}

VulkanPathTracingRendererRTCore::~VulkanPathTracingRendererRTCore()
{
	this->clear();
}

void VulkanPathTracingRendererRTCore::init()
{
	this->context_manager.enable_ray_tracing = true;
	this->context_manager.init();
	auto context_manager_sptr = std::make_shared<ContextManager>(this->context_manager);

	this->getFeatureProperty();
}

void VulkanPathTracingRendererRTCore::clear()
{
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
	VkAccelerationStructureGeometryTrianglesDataKHR triangle_data{};
	triangle_data.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	triangle_data.pNext = nullptr;
	triangle_data.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	triangle_data.vertexData.deviceAddress = this->vertex_buffer_manager.getBufferAddress();
	triangle_data.vertexStride = sizeof(Vertex);
	triangle_data.maxVertex = this->vertex_buffer_manager.vertices.size();
	triangle_data.indexType = VK_INDEX_TYPE_UINT32;
	triangle_data.indexData.deviceAddress = this->index_buffer_manager.getBufferAddress();
	triangle_data.transformData.deviceAddress = this->model_matrix_manager.getBufferAddress();

	VkAccelerationStructureGeometryKHR geometry{};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.pNext = nullptr;
	geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	geometry.geometry.triangles = triangle_data;
	geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

	std::vector<VkAccelerationStructureBuildRangeInfoKHR> build_ranges{};
	for (size_t i = 0; i < this->indirect_buffer_manager.commands.size(); i++)
	{
		auto& command = this->indirect_buffer_manager.commands[i];

		VkAccelerationStructureBuildRangeInfoKHR build_range{};
		build_range.primitiveCount = command.indexCount / 3;
		build_range.primitiveOffset = command.firstIndex * sizeof(uint32_t);
		build_range.firstVertex = command.vertexOffset;
		build_range.transformOffset = i * sizeof(VkTransformMatrixKHR);

		build_ranges.push_back(build_range);
	}

	VkAccelerationStructureBuildSizesInfoKHR build_size{};


	VkAccelerationStructureBuildGeometryInfoKHR build_geometry{};
	build_geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build_geometry.pNext = nullptr;
	build_geometry.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	build_geometry.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	build_geometry.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	build_geometry.srcAccelerationStructure = VK_NULL_HANDLE;
    build_geometry.dstAccelerationStructure;
    build_geometry.geometryCount = 1;
	build_geometry.pGeometries = &geometry;
    build_geometry.ppGeometries = nullptr;
    build_geometry.scratchData;

	//vkCmdBuildAccelerationStructuresKHR();
	//vkCreateAccelerationStructureKHR();
}

void VulkanPathTracingRendererRTCore::createAcceleration()
{
	VkAccelerationStructureCreateInfoKHR;
	VkAccelerationStructureGeometryKHR;
}