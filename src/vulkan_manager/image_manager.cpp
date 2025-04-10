#include <image_manager.h>

#define STB_IMAGE_IMPLEMENTATION

ImageManager::ImageManager(const ContextManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager)
{
	this->pContentManager = pContentManager;
	this->pCommandManager = pCommandManager;
}

void ImageManager::setExtent(const VkExtent2D& extent)
{
	this->extent = {extent.width, extent.height, 1};
}

void ImageManager::createImage(const VkExtent3D& extent,
							   const VkFormat format,
							   const VkImageTiling tiling,
							   const VkImageUsageFlags usage,
							   const VkMemoryPropertyFlags properties,
							   VkImage& image,
							   VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.pNext = nullptr;
	imageInfo.flags = 0;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = format;
	imageInfo.extent = extent;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = tiling;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.queueFamilyIndexCount = 0;
	imageInfo.pQueueFamilyIndices = nullptr;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (vkCreateImage(pContentManager->device, &imageInfo, nullptr, &image) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(pContentManager->device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(pContentManager->device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate image memory!");
	}

	vkBindImageMemory(pContentManager->device, image, imageMemory, 0);
}

void ImageManager::copyBufferToImage(VkBuffer buffer, VkImage image, const VkExtent3D& extent)
{
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = {0, 0, 0};
	region.imageExtent = extent;

	VkCommandBuffer commandBuffer = pCommandManager->beginTransferCommands();

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	pCommandManager->endTransferCommands(commandBuffer);
}

void ImageManager::copyImageToBuffer(VkImage image, VkBuffer buffer, const VkExtent3D& extent)
{
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = {0, 0, 0};
	region.imageExtent = extent;

	VkCommandBuffer commandBuffer = pCommandManager->beginTransferCommands();

	vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);

	pCommandManager->endTransferCommands(commandBuffer);
}

void ImageManager::transformLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = nullptr;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask =
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = 0;

		sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{

		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else
	{
		throw std::invalid_argument("Unsupported layout transition!");
	}

	VkCommandBuffer commandBuffer = pCommandManager->beginTransferCommands();

	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	pCommandManager->endTransferCommands(commandBuffer);
}

void ImageManager::loadImage(const std::string& imagePath,
							 const VkImageLayout layout,
							 VkImage& image,
							 VkDeviceMemory& imageMemory)
{
	int width, height, channels;
	stbi_uc* pixels = stbi_load(imagePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	VkDeviceSize imageSize = width * height * 4;
	auto extent = VkExtent3D{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};

	if (!pixels)
	{
		throw std::runtime_error("Failed to load texture image!");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(imageSize,
				 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				 stagingBuffer,
				 stagingBufferMemory);

	void* data;
	vkMapMemory(pContentManager->device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(pContentManager->device, stagingBufferMemory);

	createImage(extent,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				image,
				imageMemory);

	transformLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer, image, extent);
	transformLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout);

	vkDestroyBuffer(pContentManager->device, stagingBuffer, nullptr);
	vkFreeMemory(pContentManager->device, stagingBufferMemory, nullptr);

	stbi_image_free(pixels);
}

VkImageView ImageManager::createView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.pNext = nullptr;
	viewInfo.flags = 0;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(pContentManager->device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture image view!");
	}

	return imageView;
}

DepthImageManager::DepthImageManager(const ContextManagerSPtr& pContentManager,
									 const CommandManagerSPtr& pCommandManager)
{
	this->pContentManager = pContentManager;
	this->pCommandManager = pCommandManager;
}

void DepthImageManager::init()
{
	VkFormat depthFormat =
		findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT});

	createImage(this->extent,
				depthFormat,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				this->image,
				this->imageMemory);
	this->imageView = createView(this->image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	transformLayout(
		this->image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void DepthImageManager::clear()
{
	vkDestroyImage(pContentManager->device, this->image, nullptr);
	vkFreeMemory(pContentManager->device, this->imageMemory, nullptr);
	vkDestroyImageView(pContentManager->device, this->imageView, nullptr);
}

VkFormat DepthImageManager::findSupportedFormat(const std::vector<VkFormat>& candidates)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(pContentManager->physical_device, format, &props);

		if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) ==
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			this->format = format;
			return format;
		}
	}

	throw std::runtime_error("Failed to find supported format!");
}

StorageImageManager::StorageImageManager(const ContextManagerSPtr& pContentManager,
										 const CommandManagerSPtr& pCommandManager)
{
	this->pContentManager = pContentManager;
	this->pCommandManager = pCommandManager;
}

void StorageImageManager::init()
{
	createImage(this->extent,
				VK_FORMAT_R32G32B32A32_SFLOAT,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				this->image,
				this->imageMemory);
	this->imageView = createView(this->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

	transformLayout(this->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
}

void StorageImageManager::clear()
{
	vkDestroyImage(pContentManager->device, this->image, nullptr);
	vkFreeMemory(pContentManager->device, this->imageMemory, nullptr);
	vkDestroyImageView(pContentManager->device, this->imageView, nullptr);
}

void StorageImageManager::getData(VkBuffer& stagingBuffer, VkDeviceMemory& stagingBufferMemory)
{
	transformLayout(
		this->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	createBuffer(this->extent.width * this->extent.height * 4 * sizeof(float),
				 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				 stagingBuffer,
				 stagingBufferMemory);

	copyImageToBuffer(this->image, stagingBuffer, this->extent);
}

PointLightShadowMapImageManager::PointLightShadowMapImageManager(const ContextManagerSPtr& pContentManager,
																 const CommandManagerSPtr& pCommandManager)
{
	this->pContentManager = pContentManager;
	this->pCommandManager = pCommandManager;
}

void PointLightShadowMapImageManager::init()
{
	createImageResource();
	createSampler();
}

void PointLightShadowMapImageManager::clear()
{
	vkDestroyImage(pContentManager->device, this->image, nullptr);
	vkFreeMemory(pContentManager->device, this->memory, nullptr);
	vkDestroyImageView(pContentManager->device, this->view, nullptr);
	vkDestroySampler(pContentManager->device, this->sampler, nullptr);
}

void PointLightShadowMapImageManager::createImageResource()
{
	VkImageCreateInfo image_information{};
	image_information.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_information.pNext = nullptr;
	image_information.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	image_information.imageType = VK_IMAGE_TYPE_2D;
	image_information.format = VK_FORMAT_D32_SFLOAT;
	image_information.extent = VkExtent3D{this->width, this->height, 1};
	image_information.mipLevels = static_cast<uint32_t>(1);
	image_information.arrayLayers = static_cast<uint32_t>(6 * this->light_number);
	image_information.samples = VK_SAMPLE_COUNT_1_BIT;
	image_information.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_information.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_information.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_information.queueFamilyIndexCount = static_cast<uint32_t>(0);
	image_information.pQueueFamilyIndices = nullptr;
	image_information.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (vkCreateImage(pContentManager->device, &image_information, nullptr, &this->image) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image!");
	}

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(pContentManager->device, this->image, &memory_requirements);

	VkMemoryAllocateInfo alloc_information{};
	alloc_information.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_information.allocationSize = memory_requirements.size;
	alloc_information.memoryTypeIndex =
		findMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(pContentManager->device, &alloc_information, nullptr, &this->memory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate image memory!");
	}

	vkBindImageMemory(pContentManager->device, this->image, this->memory, 0);

	VkImageViewCreateInfo view_information{};
	view_information.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_information.pNext = nullptr;
	view_information.flags = 0;
	view_information.image = this->image;
	view_information.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
	view_information.format = VK_FORMAT_D32_SFLOAT;
	view_information.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_information.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_information.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_information.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_information.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	view_information.subresourceRange.baseMipLevel = static_cast<uint32_t>(0);
	view_information.subresourceRange.levelCount = static_cast<uint32_t>(1);
	view_information.subresourceRange.baseArrayLayer = static_cast<uint32_t>(0);
	view_information.subresourceRange.layerCount = static_cast<uint32_t>(6 * this->light_number);

	if (vkCreateImageView(pContentManager->device, &view_information, nullptr, &this->view) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image view!");
	}

	transformLayout(
		this->image, VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void PointLightShadowMapImageManager::createSampler()
{
	VkSamplerCreateInfo sampler_information{};
	sampler_information.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_information.pNext = nullptr;
	sampler_information.flags = 0;
	sampler_information.magFilter = VK_FILTER_LINEAR;
	sampler_information.minFilter = VK_FILTER_LINEAR;
	sampler_information.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler_information.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_information.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_information.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_information.mipLodBias = 0.0f;
	sampler_information.anisotropyEnable = VK_FALSE;
	sampler_information.maxAnisotropy = 1.0f;
	sampler_information.compareEnable = VK_TRUE;
	sampler_information.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	sampler_information.minLod = 0.0f;
	sampler_information.maxLod = 0.0f;
	sampler_information.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	sampler_information.unnormalizedCoordinates = VK_FALSE;

	if (vkCreateSampler(pContentManager->device, &sampler_information, nullptr, &this->sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture sampler!");
	}
}