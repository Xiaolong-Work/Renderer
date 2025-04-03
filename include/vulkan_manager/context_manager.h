#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

class ContextManager
{
public:
	ContextManager() = default;

	void init();
	void clear();

	void setExtent(const VkExtent2D& extent);

	GLFWwindow* window;
	VkInstance instance;
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	VkDevice device;

	uint32_t graphicsFamily;
	uint32_t transferFamily;
	uint32_t presentFamily;
	uint32_t computeFamily;

	VkQueue graphicsQueue;
	VkQueue transferQueue;
	VkQueue presentQueue;
	VkQueue computeQueue;

	VkSurfaceKHR surface;

	bool enable_ray_tracing{false};
	bool enable_window_resize{true};

protected:
	void createWindow();

	void createInstance();

	std::vector<const char*> getRequiredInstanceExtensions();
	bool checkInstanceExtensionSupport();

	std::vector<const char*> getRequiredValidationLayers();
	bool checkValidationLayerSupport();

	void createDebugMessenger();

	void choosePhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice physical_device);

	std::vector<const char*> getRequiredDeviceExtensions();
	bool checkDeviceExtensionSupport(VkPhysicalDevice physical_device);

	void createLogicalDevice();

	void createSurface();

private:
#ifdef NDEBUG
	bool enableValidationLayers = false;
#else
	bool enableValidationLayers = true;
#endif
	uint32_t windowWidth = 1024;
	uint32_t windowHeight = 1024;

	VkDebugUtilsMessengerEXT debugMessenger;
};

typedef std::shared_ptr<ContextManager> ContextManagerSPtr;