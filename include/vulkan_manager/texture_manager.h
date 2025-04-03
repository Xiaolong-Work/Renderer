#pragma once

#include <stb_image.h>

#include <stdexcept>
#include <string>
#include <vector>

#include <buffer_manager.h>
#include <image_manager.h>

class TextureManager : public ImageManager
{
public:
	TextureManager() = default;
	TextureManager(const ContextManagerSPtr& pContentManager, const CommandManagerSPtr& command_manager);

	void init();
	void clear();

	void createTexture(const std::string& imagePath);

	void createEmptyTexture();

	void createSampler();

	std::vector<VkImage> images;
	std::vector<VkDeviceMemory> imageMemories;
	std::vector<VkImageView> imageViews;

	VkSampler sampler;
};

typedef std::shared_ptr<TextureManager> TextureManagerSPtr;
