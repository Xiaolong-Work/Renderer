#include <image_manager.h>

#define STB_IMAGE_IMPLEMENTATION

ImageManager::ImageManager(const ContextManagerSPtr& context_manager_sptr,
						   const CommandManagerSPtr& command_manager_sptr)
{
	this->context_manager_sptr = context_manager_sptr;
	this->command_manager_sptr = command_manager_sptr;
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
							   VkDeviceMemory& memory)
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

	if (vkCreateImage(context_manager_sptr->device, &imageInfo, nullptr, &image) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(context_manager_sptr->device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(context_manager_sptr->device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate image memory!");
	}

	vkBindImageMemory(context_manager_sptr->device, image, memory, 0);
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

	VkCommandBuffer commandBuffer = command_manager_sptr->beginTransferCommands();

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	command_manager_sptr->endTransferCommands(commandBuffer);
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

	VkCommandBuffer commandBuffer = command_manager_sptr->beginTransferCommands();

	vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);

	command_manager_sptr->endTransferCommands(commandBuffer);
}

void ImageManager::transformLayout(
	VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount)
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
	barrier.subresourceRange.layerCount = layerCount;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
		oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
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
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
			 newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
			 newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
			 newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
			 newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		throw std::invalid_argument("Unsupported layout transition!");
	}

	VkCommandBuffer commandBuffer = command_manager_sptr->beginTransferCommands();

	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	command_manager_sptr->endTransferCommands(commandBuffer);
}

void ImageManager::createDeviceLocalImage(const VkDeviceSize size,
										  const VkExtent3D extent,
										  const void* data,
										  const VkImageLayout layout,
										  VkImage& image,
										  VkDeviceMemory& memory)
{
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	createBuffer(size,
				 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				 staging_buffer,
				 staging_buffer_memory);

	void* temp;
	vkMapMemory(context_manager_sptr->device, staging_buffer_memory, 0, size, 0, &temp);
	memcpy(temp, data, static_cast<size_t>(size));
	vkUnmapMemory(context_manager_sptr->device, staging_buffer_memory);

	createImage(extent,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				image,
				memory);

	transformLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(staging_buffer, image, extent);
	transformLayout(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layout);

	vkDestroyBuffer(context_manager_sptr->device, staging_buffer, nullptr);
	vkFreeMemory(context_manager_sptr->device, staging_buffer_memory, nullptr);
}

void ImageManager::loadImage(const std::string& image_path,
							 const VkImageLayout layout,
							 VkImage& image,
							 VkDeviceMemory& image_memory)
{
	int width, height, channels;
	stbi_uc* pixels = stbi_load(image_path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	VkDeviceSize image_size = width * height * 4;
	VkExtent3D extent = VkExtent3D{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};

	if (!pixels)
	{
		throw std::runtime_error("Failed to load texture image!");
	}

	createDeviceLocalImage(image_size, extent, pixels, layout, image, image_memory);

	stbi_image_free(pixels);
}

VkImageView ImageManager::createView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags)
{
	VkImageViewCreateInfo view_create_information{};
	view_create_information.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_information.pNext = nullptr;
	view_create_information.flags = 0;
	view_create_information.image = image;
	view_create_information.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_create_information.format = format;
	view_create_information.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_information.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_information.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_information.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_information.subresourceRange.aspectMask = aspect_flags;
	view_create_information.subresourceRange.baseMipLevel = 0;
	view_create_information.subresourceRange.levelCount = 1;
	view_create_information.subresourceRange.baseArrayLayer = 0;
	view_create_information.subresourceRange.layerCount = 1;

	VkImageView image_view;
	if (vkCreateImageView(context_manager_sptr->device, &view_create_information, nullptr, &image_view) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture image view!");
	}

	return image_view;
}

void ImageManager::createSampler(const VkFilter magnify,
								 const VkFilter minify,
								 const VkSamplerMipmapMode mipmap,
								 const VkSamplerAddressMode mode_u,
								 const VkSamplerAddressMode mode_v,
								 const VkSamplerAddressMode mode_w,
								 VkSampler& sampler,
								 const bool anisotropy,
								 const bool compare)
{

	VkSamplerCreateInfo sampler_information{};
	sampler_information.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_information.pNext = nullptr;
	sampler_information.flags = 0;
	sampler_information.magFilter = magnify;
	sampler_information.minFilter = minify;
	sampler_information.mipmapMode = mipmap;
	sampler_information.addressModeU = mode_u;
	sampler_information.addressModeV = mode_v;
	sampler_information.addressModeW = mode_w;
	sampler_information.mipLodBias = 0.0f;
	sampler_information.minLod = 0.0f;
	sampler_information.maxLod = 0.0f;
	sampler_information.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	sampler_information.unnormalizedCoordinates = VK_FALSE;

	if (anisotropy)
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(context_manager_sptr->physical_device, &properties);
		sampler_information.anisotropyEnable = VK_TRUE;
		sampler_information.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	}
	else
	{

		sampler_information.anisotropyEnable = VK_FALSE;
		sampler_information.maxAnisotropy = 1.0f;
	}

	if (compare)
	{
		sampler_information.compareEnable = VK_TRUE;
		sampler_information.compareOp = VK_COMPARE_OP_LESS;
	}
	else
	{
		sampler_information.compareEnable = VK_FALSE;
		sampler_information.compareOp = VK_COMPARE_OP_ALWAYS;
	}

	if (vkCreateSampler(context_manager_sptr->device, &sampler_information, nullptr, &sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture sampler!");
	}
}

void ImageManager::createSampler(const VkFilter filter,
								 const VkSamplerMipmapMode mipmap,
								 const VkSamplerAddressMode mode,
								 VkSampler& sampler,
								 const bool anisotropy,
								 const bool compare)
{
	createSampler(filter, filter, mipmap, mode, mode, mode, sampler, anisotropy, compare);
}

DepthImageManager::DepthImageManager(const ContextManagerSPtr& context_manager_sptr,
									 const CommandManagerSPtr& command_manager_sptr)
{
	this->context_manager_sptr = context_manager_sptr;
	this->command_manager_sptr = command_manager_sptr;
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
				this->memory);
	this->view = createView(this->image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	transformLayout(
		this->image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void DepthImageManager::clear()
{
	vkDestroyImage(context_manager_sptr->device, this->image, nullptr);
	vkFreeMemory(context_manager_sptr->device, this->memory, nullptr);
	vkDestroyImageView(context_manager_sptr->device, this->view, nullptr);
}

VkFormat DepthImageManager::findSupportedFormat(const std::vector<VkFormat>& candidates)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(context_manager_sptr->physical_device, format, &props);

		if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) ==
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			this->format = format;
			return format;
		}
	}

	throw std::runtime_error("Failed to find supported format!");
}

StorageImageManager::StorageImageManager(const ContextManagerSPtr& context_manager_sptr,
										 const CommandManagerSPtr& command_manager_sptr)
{
	this->context_manager_sptr = context_manager_sptr;
	this->command_manager_sptr = command_manager_sptr;
}

void StorageImageManager::init()
{
	createImage(this->extent,
				VK_FORMAT_R32G32B32A32_SFLOAT,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				this->image,
				this->memory);
	this->view = createView(this->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

	transformLayout(this->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
}

void StorageImageManager::clear()
{
	vkDestroyImage(context_manager_sptr->device, this->image, nullptr);
	vkFreeMemory(context_manager_sptr->device, this->memory, nullptr);
	vkDestroyImageView(context_manager_sptr->device, this->view, nullptr);
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

PointLightShadowMapImageManager::PointLightShadowMapImageManager(const ContextManagerSPtr& context_manager_sptr,
																 const CommandManagerSPtr& command_manager_sptr)
{
	this->context_manager_sptr = context_manager_sptr;
	this->command_manager_sptr = command_manager_sptr;
}

void PointLightShadowMapImageManager::init()
{
	createImageResource();
	createSampler();
}

void PointLightShadowMapImageManager::clear()
{
	vkDestroyImage(context_manager_sptr->device, this->image, nullptr);
	vkFreeMemory(context_manager_sptr->device, this->memory, nullptr);
	vkDestroyImageView(context_manager_sptr->device, this->view, nullptr);
	vkDestroyImage(context_manager_sptr->device, this->depth_image, nullptr);
	vkFreeMemory(context_manager_sptr->device, this->depth_memory, nullptr);
	vkDestroyImageView(context_manager_sptr->device, this->depth_view, nullptr);
	vkDestroySampler(context_manager_sptr->device, this->sampler, nullptr);
}

void PointLightShadowMapImageManager::createImageResource()
{
	VkImageCreateInfo image_information{};
	image_information.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_information.pNext = nullptr;
	image_information.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	image_information.imageType = VK_IMAGE_TYPE_2D;
	image_information.format = VK_FORMAT_R32_SFLOAT;
	image_information.extent = VkExtent3D{this->width, this->height, 1};
	image_information.mipLevels = 1;
	image_information.arrayLayers = static_cast<uint32_t>(6 * this->light_number);
	image_information.samples = VK_SAMPLE_COUNT_1_BIT;
	image_information.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_information.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_information.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_information.queueFamilyIndexCount = 0u;
	image_information.pQueueFamilyIndices = nullptr;
	image_information.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if (vkCreateImage(context_manager_sptr->device, &image_information, nullptr, &this->image) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image!");
	}

	image_information.format = VK_FORMAT_D32_SFLOAT;
	image_information.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	if (vkCreateImage(context_manager_sptr->device, &image_information, nullptr, &this->depth_image) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image!");
	}

	auto bindMemory = [this](VkImage& image, VkDeviceMemory& memory) {
		VkMemoryRequirements memory_requirements;
		vkGetImageMemoryRequirements(context_manager_sptr->device, image, &memory_requirements);

		VkMemoryAllocateInfo allocate_information{};
		allocate_information.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocate_information.allocationSize = memory_requirements.size;
		allocate_information.memoryTypeIndex =
			findMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(context_manager_sptr->device, &allocate_information, nullptr, &memory) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate image memory!");
		}

		vkBindImageMemory(context_manager_sptr->device, image, memory, 0);
	};
	bindMemory(this->image, this->memory);
	bindMemory(this->depth_image, this->depth_memory);

	VkImageViewCreateInfo view_information{};
	view_information.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_information.pNext = nullptr;
	view_information.flags = 0;
	view_information.image = this->image;
	view_information.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
	view_information.format = VK_FORMAT_R32_SFLOAT;
	view_information.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_information.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_information.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_information.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_information.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view_information.subresourceRange.baseMipLevel = 0;
	view_information.subresourceRange.levelCount = 1;
	view_information.subresourceRange.baseArrayLayer = 0;
	view_information.subresourceRange.layerCount = static_cast<uint32_t>(6 * this->light_number);
	if (vkCreateImageView(context_manager_sptr->device, &view_information, nullptr, &this->view) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image view!");
	}

	view_information.image = this->depth_image;
	view_information.format = VK_FORMAT_D32_SFLOAT;
	view_information.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	if (vkCreateImageView(context_manager_sptr->device, &view_information, nullptr, &this->depth_view) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image view!");
	}
}

void PointLightShadowMapImageManager::createSampler()
{
	VkSamplerCreateInfo sampler_information{};
	sampler_information.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_information.pNext = nullptr;
	sampler_information.flags = 0;
	sampler_information.magFilter = VK_FILTER_NEAREST;
	sampler_information.minFilter = VK_FILTER_NEAREST;
	sampler_information.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler_information.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_information.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_information.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_information.mipLodBias = 0.0f;
	sampler_information.anisotropyEnable = VK_FALSE;
	sampler_information.maxAnisotropy = 1.0f;
	sampler_information.compareEnable = VK_FALSE;
	sampler_information.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_information.minLod = 0.0f;
	sampler_information.maxLod = 0.0f;
	sampler_information.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	sampler_information.unnormalizedCoordinates = VK_FALSE;

	if (vkCreateSampler(context_manager_sptr->device, &sampler_information, nullptr, &this->sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture sampler!");
	}
}