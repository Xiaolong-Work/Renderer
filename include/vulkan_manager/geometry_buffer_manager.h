#pragma once

#include <image_manager.h>

class GeometryBufferManager : public ImageManager
{
public:
	GeometryBufferManager() = default;
	GeometryBufferManager(const ContextManagerSPtr& context_manager_sptr,
						  const CommandManagerSPtr& command_manager_sptr);

	void init();
	void clear();

	std::vector<VkDescriptorSetLayoutBinding> getLayoutBindings(const uint32_t begin_binding,
																const VkShaderStageFlags flag);

	std::vector<VkWriteDescriptorSet> getWriteInformations(const uint32_t begin_binding);

	std::vector<std::pair<VkDescriptorSetLayoutBinding, VkWriteDescriptorSet>> getDescriptors(
		const uint32_t binding, const VkShaderStageFlags flag);

	void setUsage(const VkImageUsageFlags usage);

	std::vector<VkFormat> getFormats()
	{
		std::vector<VkFormat> formats{
			this->position_format, this->normal_format, this->albedo_format, this->material_format};
		return formats;
	}

	std::vector<VkImageView> getViews()
	{
		std::vector<VkImageView> views{this->position_view, this->normal_view, this->albedo_view, this->material_view};
		return views;
	}

protected:
	void createImageResource();

	void createSampler();

private:
	void createGeometryBufferImage(const VkFormat format, VkImage& image, VkDeviceMemory& memory, VkImageView& view);

	/* Position in world coordinate system */
	VkImage position{};
	VkDeviceMemory position_memory{};
	VkImageView position_view{};
	VkFormat position_format{VK_FORMAT_R32G32B32A32_SFLOAT};

	/* Normal in world coordinate system */
	VkImage normal{};
	VkDeviceMemory normal_memory{};
	VkImageView normal_view{};
	VkFormat normal_format{VK_FORMAT_R32G32B32A32_SFLOAT};

	/* The base color of the shading point */
	VkImage albedo{};
	VkDeviceMemory albedo_memory{};
	VkImageView albedo_view{};
	VkFormat albedo_format{VK_FORMAT_R32G32B32A32_SFLOAT};

	/* The material of the shading point */
	VkImage material{};
	VkDeviceMemory material_memory{};
	VkImageView material_view{};
	VkFormat material_format{VK_FORMAT_R32G32B32A32_SFLOAT};

	VkSampler sampler{};

	std::vector<VkDescriptorImageInfo> descriptor_image{};

	VkImageUsageFlags usage{VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT};
};

typedef std::shared_ptr<GeometryBufferManager> GeometryBufferManagerSPtr;
