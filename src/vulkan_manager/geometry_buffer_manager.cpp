#include <geometry_buffer_manager.h>

GeometryBufferManager::GeometryBufferManager(const ContextManagerSPtr& context_manager_sptr,
											 const CommandManagerSPtr& command_manager_sptr)
{
	this->context_manager_sptr = context_manager_sptr;
	this->command_manager_sptr = command_manager_sptr;
}

void GeometryBufferManager::init()
{
	createImageResource();
}

void GeometryBufferManager::clear()
{
	vkDestroyImage(context_manager_sptr->device, this->position, nullptr);
	vkDestroyImage(context_manager_sptr->device, this->normal, nullptr);
	vkDestroyImage(context_manager_sptr->device, this->albedo, nullptr);
	vkDestroyImage(context_manager_sptr->device, this->material, nullptr);

	vkDestroyImageView(context_manager_sptr->device, this->position_view, nullptr);
	vkDestroyImageView(context_manager_sptr->device, this->normal_view, nullptr);
	vkDestroyImageView(context_manager_sptr->device, this->albedo_view, nullptr);
	vkDestroyImageView(context_manager_sptr->device, this->material_view, nullptr);

	vkDestroySampler(context_manager_sptr->device, this->sampler, nullptr);
}

std::vector<VkDescriptorSetLayoutBinding> GeometryBufferManager::getLayoutBindings(const uint32_t begin_binding,
																				   const VkShaderStageFlags flag)
{
	std::vector<VkDescriptorSetLayoutBinding> layout_bindings{};

	VkDescriptorType type{};
	if ((this->usage & VK_IMAGE_USAGE_STORAGE_BIT) == VK_IMAGE_USAGE_STORAGE_BIT)
	{
		type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	}
	else
	{
		type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	}

	layout_bindings.resize(4);
	for (size_t i = 0; i < 4; i++)
	{
		layout_bindings[i].binding = begin_binding + i;
		layout_bindings[i].descriptorType = type;
		layout_bindings[i].descriptorCount = 1;
		layout_bindings[i].stageFlags = flag;
		layout_bindings[i].pImmutableSamplers = nullptr;
	}

	return layout_bindings;
}

std::vector<VkWriteDescriptorSet> GeometryBufferManager::getWriteInformations(const uint32_t binding)
{
	VkDescriptorType type{};
	VkImageLayout layout{};
	if ((this->usage & VK_IMAGE_USAGE_STORAGE_BIT) == VK_IMAGE_USAGE_STORAGE_BIT)
	{
		type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		layout = VK_IMAGE_LAYOUT_GENERAL;
	}
	else
	{
		type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	this->descriptor_image.resize(4);
	std::vector<VkWriteDescriptorSet> texture_writes{};
	texture_writes.resize(4);
	for (size_t i = 0; i < 4; i++)
	{
		this->descriptor_image[i].sampler = sampler;
		this->descriptor_image[i].imageLayout = layout;

		texture_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		texture_writes[i].pNext = nullptr;
		texture_writes[i].dstSet = VK_NULL_HANDLE;
		texture_writes[i].dstBinding = binding + i;
		texture_writes[i].dstArrayElement = 0;
		texture_writes[i].descriptorCount = 1;
		texture_writes[i].descriptorType = type;
		texture_writes[i].pBufferInfo = nullptr;
		texture_writes[i].pImageInfo = &this->descriptor_image[i];
		texture_writes[i].pTexelBufferView = nullptr;
	}
	this->descriptor_image[0].imageView = position_view;
	this->descriptor_image[1].imageView = normal_view;
	this->descriptor_image[2].imageView = albedo_view;
	this->descriptor_image[3].imageView = material_view;

	return texture_writes;
}

std::vector<std::pair<VkDescriptorSetLayoutBinding, VkWriteDescriptorSet>> GeometryBufferManager::getDescriptors(
	const uint32_t begin_binding, const VkShaderStageFlags flag)
{
	std::vector<std::pair<VkDescriptorSetLayoutBinding, VkWriteDescriptorSet>> result{};
	auto bindings = getLayoutBindings(begin_binding, flag);
	auto writes = getWriteInformations(begin_binding);
	for (size_t i = 0; i < bindings.size(); i++)
	{
		result.push_back(std::make_pair(bindings[i], writes[i]));
	}
	return result;
}

void GeometryBufferManager::setUsage(const VkImageUsageFlags usage)
{
	this->usage = usage;
}

void GeometryBufferManager::createImageResource()
{
	createGeometryBufferImage(
		VK_FORMAT_R32G32B32A32_SFLOAT, this->position, this->position_memory, this->position_view);

	createGeometryBufferImage(VK_FORMAT_R32G32B32A32_SFLOAT, this->normal, this->normal_memory, this->normal_view);

	createGeometryBufferImage(VK_FORMAT_R32G32B32A32_SFLOAT, this->albedo, this->albedo_memory, this->albedo_view);

	createGeometryBufferImage(
		VK_FORMAT_R32G32B32A32_SFLOAT, this->material, this->material_memory, this->material_view);
}

void GeometryBufferManager::createSampler()
{
	ImageManager::createSampler(
		VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, this->sampler);
}

void GeometryBufferManager::createGeometryBufferImage(const VkFormat format,
													  VkImage& image,
													  VkDeviceMemory& memory,
													  VkImageView& view)
{
	VkDescriptorType type{};
	VkImageLayout layout{};
	if ((this->usage & VK_IMAGE_USAGE_STORAGE_BIT) == VK_IMAGE_USAGE_STORAGE_BIT)
	{
		layout = VK_IMAGE_LAYOUT_GENERAL;
	}
	else
	{
		layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	createImage(this->extent,
				format,
				VK_IMAGE_TILING_OPTIMAL,
				this->usage,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				image,
				memory);

	ImageManager::transformLayout(image, format, VK_IMAGE_LAYOUT_UNDEFINED, layout);

	view = createView(image, format, VK_IMAGE_ASPECT_COLOR_BIT);
}