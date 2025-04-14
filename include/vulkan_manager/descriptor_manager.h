#pragma once

#include <array>
#include <stdexcept>
#include <vector>

#include <context_manager.h>

class DescriptorManager
{
public:
	DescriptorManager() = default;
	DescriptorManager(const ContextManagerSPtr& context_manager_sptr);

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
	ContextManagerSPtr context_manager_sptr;
};
