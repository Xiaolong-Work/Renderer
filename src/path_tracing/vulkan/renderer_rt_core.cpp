#include <renderer_rt_core.h>

VulkanPathTracingRenderRTCore::VulkanPathTracingRenderRTCore()
{
	init();
}

VulkanPathTracingRenderRTCore::~VulkanPathTracingRenderRTCore()
{
	clear();
}

void VulkanPathTracingRenderRTCore::init()
{
	this->contentManager.enableRayTracing = true;
	this->contentManager.init();
	auto pContentManager = std::make_shared<ContentManager>(this->contentManager);

	this->getFeatureProperty();

	this->commandManager = CommandManager(pContentManager);
	this->commandManager.init();
	auto pCommandManager = std::make_shared<CommandManager>(this->commandManager);

	this->swap_chain_manager = SwapChainManager(pContentManager, pCommandManager);
	this->swap_chain_manager.init();

	this->depthImageManager = DepthImageManager(pContentManager, pCommandManager);
	this->depthImageManager.setExtent(this->swap_chain_manager.extent);
	this->depthImageManager.init();

	this->vertex_shader_manager = ShaderManager(pContentManager);
	this->vertex_shader_manager.setShaderName("rasterize_vert.spv");
	this->vertex_shader_manager.init();

	this->fragment_shader_manager = ShaderManager(pContentManager);
	this->fragment_shader_manager.setShaderName("rasterize_frag.spv");
	this->fragment_shader_manager.init();
}

void VulkanPathTracingRenderRTCore::clear()
{
}

void VulkanPathTracingRenderRTCore::getFeatureProperty()
{
	this->property.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 deviceProperties{};
	deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProperties.pNext = &this->property;
	vkGetPhysicalDeviceProperties2(this->contentManager.physicalDevice, &deviceProperties);

	this->feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	VkPhysicalDeviceFeatures2 deviceFeatures2{};
	deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures2.pNext = &this->feature;
	vkGetPhysicalDeviceFeatures2(this->contentManager.physicalDevice, &deviceFeatures2);
}

void VulkanPathTracingRenderRTCore::createAcceleration()
{
	VkAccelerationStructureCreateInfoKHR;
}