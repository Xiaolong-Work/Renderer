#include <texture_manager.h>

TextureManager::TextureManager(const ContextManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager)
{
	this->pContentManager = pContentManager;
	this->pCommandManager = pCommandManager;
}

void TextureManager::createTexture(const std::string& imagePath)
{
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;

	loadImage(imagePath, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image, imageMemory);
	imageView = createView(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

	this->images.push_back(image);
	this->imageMemories.push_back(imageMemory);
	this->imageViews.push_back(imageView);
}

void TextureManager::createEmptyTexture()
{
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;

	createImage(this->extent,
				VK_FORMAT_R32G32B32A32_SFLOAT,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				image,
				imageMemory);

	imageView = createView(image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

	transformLayout(
		image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	this->images.push_back(image);
	this->imageMemories.push_back(imageMemory);
	this->imageViews.push_back(imageView);
}

void TextureManager::createSampler()
{
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(pContentManager->physical_device, &properties);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.pNext = nullptr;
	samplerInfo.flags = 0;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	if (vkCreateSampler(pContentManager->device, &samplerInfo, nullptr, &this->sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture sampler!");
	}
}

void TextureManager::init()
{
	this->extent = {1, 1, 1};
	createSampler();
}

void TextureManager::clear()
{
	vkDestroySampler(pContentManager->device, this->sampler, nullptr);

	for (auto& imageView : this->imageViews)
	{
		vkDestroyImageView(pContentManager->device, imageView, nullptr);
	}
	for (auto& image : this->images)
	{
		vkDestroyImage(pContentManager->device, image, nullptr);
	}
	for (auto& imageMemory : this->imageMemories)
	{
		vkFreeMemory(pContentManager->device, imageMemory, nullptr);
	}

	this->imageViews.clear();
	this->images.clear();
	this->imageMemories.clear();
}
