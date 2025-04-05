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

	this->command_manager = CommandManager(context_manager_sptr);
	this->command_manager.init();
	auto command_manager_sptr = std::make_shared<CommandManager>(this->command_manager);

	this->vertex_buffer_manager = VertexBufferManager(context_manager_sptr, command_manager_sptr);
	this->vertex_buffer_manager.enable_ray_tracing = true;
	this->index_buffer_manager = IndexBufferManager(context_manager_sptr, command_manager_sptr);

	this->swap_chain_manager = SwapChainManager(context_manager_sptr, command_manager_sptr);
	this->swap_chain_manager.init();

	this->vertex_shader_manager = ShaderManager(context_manager_sptr);
	this->vertex_shader_manager.setShaderName("rasterize_vert.spv");
	this->vertex_shader_manager.init();

	this->fragment_shader_manager = ShaderManager(context_manager_sptr);
	this->fragment_shader_manager.setShaderName("rasterize_frag.spv");
	this->fragment_shader_manager.init();
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

	VkBufferDeviceAddressInfo info{};

    triangle_data.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	triangle_data.pNext = nullptr;
	triangle_data.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	triangle_data.vertexData.deviceAddress;
    triangle_data.vertexStride;
    triangle_data.maxVertex;
    triangle_data.indexType;
    triangle_data.indexData;
    triangle_data.transformData.deviceAddress = 0;
}

void VulkanPathTracingRendererRTCore::createAcceleration()
{
	VkAccelerationStructureCreateInfoKHR;
	VkAccelerationStructureGeometryKHR;
}