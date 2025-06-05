#pragma once

#include <stb_image.h>

#include <stdexcept>
#include <string>
#include <vector>

#include <buffer_manager.h>
#include <image_manager.h>

#include <texture.h>

class TextureManager : public ImageManager
{
public:
	TextureManager() = default;
	TextureManager(const ContextManagerSPtr& context_manager_sptr, const CommandManagerSPtr& command_manager_sptr);

	void init();
	void clear();

	void createTexture(const std::string& image_path);

	void createTexture(const Texture& texture);

	void createEmptyTexture();

	void createSampler(const Texture& texture);

	bool isEmpty();

	VkDescriptorSetLayoutBinding getLayoutBinding(const uint32_t binding, const VkShaderStageFlags flag);

	VkWriteDescriptorSet getWriteInformation(const uint32_t binding);

	std::pair<VkDescriptorSetLayoutBinding, VkWriteDescriptorSet> getDescriptor(const uint32_t binding,
																				const VkShaderStageFlags flag);

	std::vector<VkImage> images{};
	std::vector<VkDeviceMemory> memories{};
	std::vector<VkImageView> views{};
	std::vector<VkSampler> samplers{};

	std::vector<VkDescriptorImageInfo> descriptor_image{};
};

typedef std::shared_ptr<TextureManager> TextureManagerSPtr;
