#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <image_manager.h>

class SwapChainManager : public ImageManager
{
public:
	SwapChainManager() = default;
	SwapChainManager(const ContentManagerSPtr& pContentManager, const CommandManagerSPtr& commandManager);

	void init();
	void clear();

	void recreate();

	VkSwapchainKHR swapChain;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;
	VkFormat format;
	VkExtent2D extent;

protected:
	VkExtent2D getExtent();
	VkSurfaceFormatKHR getFormat();
	VkPresentModeKHR getPresentMode();

	void createSwapChain();

private:
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

typedef std::shared_ptr<SwapChainManager> SwapChainManagerSPtr;