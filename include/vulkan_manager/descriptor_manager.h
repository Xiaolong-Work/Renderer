#pragma once

#include <array>
#include <stdexcept>
#include <vector>

#include <content_manager.h>

class DescriptorManager
{
public:
	DescriptorManager() = default;
	DescriptorManager(const ContentManagerSPtr& pContentManager);

	void init();
	void clear();

	void createDescriptorSetLayout();
	void createDescriptorPool();
	void createDescriptorSet();

	std::vector<VkDescriptorSetLayoutBinding> bindings{};
	std::vector<VkDescriptorPoolSize> poolSizes{};

	VkDescriptorSetLayout layout;
	VkDescriptorPool pool;
	VkDescriptorSet set;

private:
	ContentManagerSPtr pContentManager;
};
