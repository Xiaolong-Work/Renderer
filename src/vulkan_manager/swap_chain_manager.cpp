#include <swap_chain_manager.h>

SwapChainManager::SwapChainManager(const ContentManagerSPtr& pContentManager, const CommandManagerSPtr& commandManager)
{
    this->pContentManager = pContentManager;
    this->pCommandManager = commandManager;
}

void SwapChainManager::init() { create(); }

void SwapChainManager::clear()
{
    for (size_t i = 0; i < imageViews.size(); i++)
    {
        vkDestroyImageView(pContentManager->device, this->imageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(pContentManager->device, this->swapChain, nullptr);
}

VkExtent2D SwapChainManager::getExtent()
{
    /* Query surface capabilities */
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        pContentManager->physicalDevice, pContentManager->surface, &this->capabilities);

    if (this->capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return this->capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(pContentManager->window, &width, &height);

        VkExtent2D resultlExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        resultlExtent.width = std::clamp(
            resultlExtent.width, this->capabilities.minImageExtent.width, this->capabilities.maxImageExtent.width);
        resultlExtent.height = std::clamp(
            resultlExtent.height, this->capabilities.minImageExtent.height, this->capabilities.maxImageExtent.height);

        return resultlExtent;
    }
}

VkSurfaceFormatKHR SwapChainManager::getFormat()
{
    /* Query surface format */
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        pContentManager->physicalDevice, pContentManager->surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        this->formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            pContentManager->physicalDevice, pContentManager->surface, &formatCount, this->formats.data());
    }

    for (const auto& format : this->formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    return this->formats[0];
}

VkPresentModeKHR SwapChainManager::getPresentMode()
{
    /* Query rendering mode */
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        pContentManager->physicalDevice, pContentManager->surface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        this->presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            pContentManager->physicalDevice, pContentManager->surface, &presentModeCount, this->presentModes.data());
    }

    for (const auto& presentMode : this->presentModes)
    {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return presentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

void SwapChainManager::createFramebuffers() { }

void SwapChainManager::create()
{
    VkExtent2D extent = getExtent();
    VkSurfaceFormatKHR surfaceFormat = getFormat();
    VkPresentModeKHR presentMode = getPresentMode();

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.surface = pContentManager->surface;
    createInfo.imageExtent = extent;

    uint32_t imageCount = this->capabilities.minImageCount + 1;
    if (this->capabilities.maxImageCount > 0 && imageCount > this->capabilities.maxImageCount)
    {
        imageCount = this->capabilities.maxImageCount;
    }
    createInfo.minImageCount = imageCount;

    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = {pContentManager->graphicsFamily, pContentManager->presentFamily};
    if (pContentManager->graphicsFamily != pContentManager->presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = this->capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    /* Create a swap chain */
    if (vkCreateSwapchainKHR(pContentManager->device, &createInfo, nullptr, &this->swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create swap chain!");
    }

    /* Get the operation handle of the image in the swap chain */
    vkGetSwapchainImagesKHR(pContentManager->device, this->swapChain, &imageCount, nullptr);
    this->images.resize(imageCount);
    vkGetSwapchainImagesKHR(pContentManager->device, this->swapChain, &imageCount, this->images.data());

    this->format = surfaceFormat.format;
    this->extent = extent;

    this->imageViews.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++)
    {
        this->imageViews[i] = createView(this->images[i], this->format, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}