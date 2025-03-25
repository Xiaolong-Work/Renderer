#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

class ContentManager
{
public:
    ContentManager() = default;

	void setExtent(const VkExtent2D& extent);

    void init();
    void clear();

    GLFWwindow* window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkSurfaceKHR surface;

    VkQueue graphicsQueue;
    VkQueue transferQueue;
    VkQueue presentQueue;
    VkQueue computeQueue;

    uint32_t graphicsFamily;
    uint32_t transferFamily;
    uint32_t presentFamily;
    uint32_t computeFamily;

	bool enableRayTracing{false};

protected:
    void createWindow();

    std::vector<const char*> getRequiredInstanceExtensions();
    bool checkInstanceExtensionSupport();

    std::vector<const char*> getRequiredValidationLayers();
    bool checkValidationLayerSupport();

    std::vector<const char*> getRequiredDeviceExtensions();
    bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);

    void createDebugMessenger();

    void createLogicalDevice();

    void createSurface();

    void createInstance();

    void choosePhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice physicalDevice);

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

typedef std::shared_ptr<ContentManager> ContentManagerSPtr;