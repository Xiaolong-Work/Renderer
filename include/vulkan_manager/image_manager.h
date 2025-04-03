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
	ImageManager(const ContextManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager);

	void setExtent(const VkExtent2D& extent);

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

protected:
	VkExtent3D extent;
};

class DepthImageManager : public ImageManager
{
public:
	DepthImageManager() = default;
	DepthImageManager(const ContextManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager);

	void init();
	void clear();

	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;
	VkFormat format;

protected:
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates);
};

class StorageImageManager : public ImageManager
{
public:
	StorageImageManager() = default;
	StorageImageManager(const ContextManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager);

	void init();
	void clear();

	void getData(VkBuffer& stagingBuffer, VkDeviceMemory& stagingBufferMemory);

	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;
};

class PointLightShadowMapImageManager : public ImageManager
{
public:
	PointLightShadowMapImageManager() = default;
	PointLightShadowMapImageManager(const ContextManagerSPtr& pContentManager,
									const CommandManagerSPtr& pCommandManager);

	void init();
	void clear();

	VkImage image{};
	VkDeviceMemory memory{};
	VkImageView view{};
	VkSampler sampler{};

	int light_number{0};
	unsigned int width{1024};
	unsigned int height{1024};

protected:
	void createImageResource();

	void createSampler();
};
