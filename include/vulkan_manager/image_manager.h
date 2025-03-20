#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <buffer_manager.h>
#include <stb_image.h>
#include <stdexcept>

class ImageManager : public BufferManager
{
public:
    ImageManager() = default;
    ImageManager(const ContentManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager);

    void createImage(const VkExtent3D& extent,
                     const VkFormat format,
                     const VkImageTiling tiling,
                     const VkImageUsageFlags usage,
                     const VkMemoryPropertyFlags properties,
                     VkImage& image,
                     VkDeviceMemory& imageMemory);

    void copyBufferToImage(VkBuffer buffer, VkImage image, const VkExtent3D& extent);

	void copyImageToBuffer(VkImage image, VkBuffer buffer, const VkExtent3D& extent);

    void transformLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

    void loadImage(const std::string& imagePath,
                   const VkImageLayout layout,
                   VkImage& image,
                   VkDeviceMemory& imageMemory);

    VkImageView createView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	void setExtent(const VkExtent2D& extent);

protected:
    VkExtent3D extent;
};

class DepthImageManager : public ImageManager
{
public:
    DepthImageManager() = default;
    DepthImageManager(const ContentManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager);

    void init();
    void clear();

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                                 VkImageTiling tiling,
                                 VkFormatFeatureFlags features);

    VkFormat findDepthFormat();

    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;
};

class StorageImageManager : public ImageManager
{
public:
    StorageImageManager() = default;
    StorageImageManager(const ContentManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager);

	void init();
    void clear();

	void getData(VkBuffer& stagingBuffer, VkDeviceMemory& stagingBufferMemory);

	VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;
};
