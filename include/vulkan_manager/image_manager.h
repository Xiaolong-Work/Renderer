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
	ImageManager(const ContextManagerSPtr& context_manager_sptr, const CommandManagerSPtr& command_manager_sptr);

	void setExtent(const VkExtent2D& extent);

	void createImage(const VkExtent3D& extent,
					 const VkFormat format,
					 const VkImageTiling tiling,
					 const VkImageUsageFlags usage,
					 const VkMemoryPropertyFlags properties,
					 VkImage& image,
					 VkDeviceMemory& memory);

	void copyBufferToImage(VkBuffer buffer, VkImage image, const VkExtent3D& extent);

	void copyImageToBuffer(VkImage image, VkBuffer buffer, const VkExtent3D& extent);

	void transformLayout(
		VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layercount = 1);

	void createDeviceLocalImage(const VkDeviceSize size,
								const VkExtent3D extent,
								const void* data,
								const VkImageLayout layout,
								VkImage& image,
								VkDeviceMemory& memory);

	void loadImage(const std::string& imagePath,
				   const VkImageLayout layout,
				   VkImage& image,
				   VkDeviceMemory& memory);

	VkImageView createView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	void createSampler(const VkFilter magnify,
					   const VkFilter minify,
					   const VkSamplerMipmapMode mipmap,
					   const VkSamplerAddressMode mode_u,
					   const VkSamplerAddressMode mode_v,
					   const VkSamplerAddressMode mode_w,
					   VkSampler& sampler,
					   const bool anisotropy = false,
					   const bool compare = false);

	void createSampler(const VkFilter filter,
					   const VkSamplerMipmapMode mipmap,
					   const VkSamplerAddressMode mode,
					   VkSampler& sampler,
					   const bool anisotropy = false,
					   const bool compare = false);

protected:
	VkExtent3D extent;
};

class DepthImageManager : public ImageManager
{
public:
	DepthImageManager() = default;
	DepthImageManager(const ContextManagerSPtr& context_manager_sptr, const CommandManagerSPtr& command_manager_sptr);

	void init();
	void clear();

	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkFormat format;

protected:
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates);
};

class StorageImageManager : public ImageManager
{
public:
	StorageImageManager() = default;
	StorageImageManager(const ContextManagerSPtr& context_manager_sptr, const CommandManagerSPtr& command_manager_sptr);

	void init();
	void clear();

	void getData(VkBuffer& stagingBuffer, VkDeviceMemory& stagingBufferMemory);

	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
};

class PointLightShadowMapImageManager : public ImageManager
{
public:
	PointLightShadowMapImageManager() = default;
	PointLightShadowMapImageManager(const ContextManagerSPtr& context_manager_sptr,
									const CommandManagerSPtr& command_manager_sptr);

	void init();
	void clear();

	VkImage image{};
	VkDeviceMemory memory{};
	VkImageView view{};

	VkImage depth_image{};
	VkDeviceMemory depth_memory{};
	VkImageView depth_view{};

	VkSampler sampler{};

	int light_number{0};
	unsigned int width{1024};
	unsigned int height{1024};

protected:
	void createImageResource();

	void createSampler();
};
