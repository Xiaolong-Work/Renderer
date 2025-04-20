#include <texture_manager.h>

TextureManager::TextureManager(const ContextManagerSPtr& context_manager_sptr,
							   const CommandManagerSPtr& command_manager_sptr)
{
	this->context_manager_sptr = context_manager_sptr;
	this->command_manager_sptr = command_manager_sptr;
}

void TextureManager::createTexture(const std::string& image_path)
{
	VkImage image;
	VkDeviceMemory memory;
	VkImageView image_view;

	loadImage(image_path, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image, memory);
	image_view = createView(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

	this->images.push_back(image);
	this->memories.push_back(memory);
	this->views.push_back(image_view);
}

void TextureManager::createEmptyTexture()
{
	VkImage image;
	VkDeviceMemory image_memory;
	VkImageView view;

	createImage(this->extent,
				VK_FORMAT_R32G32B32A32_SFLOAT,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				image,
				image_memory);

	view = createView(image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

	transformLayout(
		image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	this->images.push_back(image);
	this->memories.push_back(image_memory);
	this->views.push_back(view);
}

void TextureManager::createTexture(const Texture& texture)
{
	VkImage image;
	VkDeviceMemory memory;
	VkImageView image_view;

	VkDeviceSize image_size = texture.width * texture.height * 4;
	VkExtent3D extent = {static_cast<uint32_t>(texture.width), static_cast<uint32_t>(texture.height), 1};

	createDeviceLocalImage(
		image_size, extent, texture.data.data(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image, memory);
	image_view = createView(image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

	this->images.push_back(image);
	this->memories.push_back(memory);
	this->views.push_back(image_view);
}

void TextureManager::createSampler(const Texture& texture)
{
	auto get_address_mode = [](SamplerAddressMode mode) {
		switch (mode)
		{
		case Undefined:
			{
				return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			}
		case Clamp_To_Edge:
			{
				return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			}
		case Mirrored_Repeat:
			{
				return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			}
		case Repeat:
			{
				return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			}
		default:
			{
				return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			}
		}
	};

	auto get_filter_mode = [](SamplerFilterMode mode) {
		switch (mode)
		{
		case SamplerFilterMode::Undefined:
			{
				return VK_FILTER_LINEAR;
			}
		case SamplerFilterMode::Nearest:
			{
				return VK_FILTER_NEAREST;
			}
		case SamplerFilterMode::Linear:
			{
				return VK_FILTER_LINEAR;
			}
		default:
			{
				return VK_FILTER_LINEAR;
			}
		}
	};

	VkSamplerAddressMode mode_u = get_address_mode(texture.address_u);
	VkSamplerAddressMode mode_v = get_address_mode(texture.address_v);
	VkSamplerAddressMode mode_w = get_address_mode(texture.address_w);

	VkFilter magnify = get_filter_mode(texture.magnify);
	VkFilter minify = get_filter_mode(texture.minify);

	VkSamplerMipmapMode mipmap;
	if (texture.mipmap == SamplerMipMapMode::Nearest)
	{
		mipmap = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}
	else
	{
		mipmap = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}

	VkSampler sampler;
	ImageManager::createSampler(magnify, minify, mipmap, mode_u, mode_v, mode_w, sampler, true);
	this->samplers.push_back(sampler);
}

bool TextureManager::isEmpty()
{
	return this->views.empty();
}

void TextureManager::init()
{
	this->extent = {1, 1, 1};
}

void TextureManager::clear()
{
	for (auto& sampler : this->samplers)
	{
		vkDestroySampler(context_manager_sptr->device, sampler, nullptr);
	}
	for (auto& view : this->views)
	{
		vkDestroyImageView(context_manager_sptr->device, view, nullptr);
	}
	for (auto& image : this->images)
	{
		vkDestroyImage(context_manager_sptr->device, image, nullptr);
	}
	for (auto& memory : this->memories)
	{
		vkFreeMemory(context_manager_sptr->device, memory, nullptr);
	}

	this->views.clear();
	this->images.clear();
	this->memories.clear();
}

VkDescriptorSetLayoutBinding TextureManager::getLayoutBinding(const uint32_t binding, const VkShaderStageFlags flag)
{
	VkDescriptorSetLayoutBinding layout_binding{};
	layout_binding.binding = binding;
	layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layout_binding.descriptorCount = static_cast<uint32_t>(this->views.size());
	layout_binding.stageFlags = flag;
	layout_binding.pImmutableSamplers = nullptr;

	return layout_binding;
}

VkWriteDescriptorSet TextureManager::getWriteInformation(const uint32_t binding)
{
	this->descriptor_image.resize(this->views.size());
	for (size_t i = 0; i < this->views.size(); i++)
	{
		this->descriptor_image[i].imageView = this->views[i];
		this->descriptor_image[i].sampler = this->samplers[i];
		this->descriptor_image[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	VkWriteDescriptorSet texture_write{};
	texture_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	texture_write.pNext = nullptr;
	texture_write.dstSet = VK_NULL_HANDLE;
	texture_write.dstBinding = binding;
	texture_write.dstArrayElement = 0;
	texture_write.descriptorCount = static_cast<uint32_t>(this->descriptor_image.size());
	texture_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	texture_write.pBufferInfo = nullptr;
	texture_write.pImageInfo = this->descriptor_image.data();
	texture_write.pTexelBufferView = nullptr;

	return texture_write;
}
