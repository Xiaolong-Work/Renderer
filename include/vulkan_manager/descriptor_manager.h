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

	void addLayoutBinding(const VkDescriptorSetLayoutBinding& binding);
	void addPoolSize(const VkDescriptorType type, const uint32_t size);
	void addWrite(const VkWriteDescriptorSet& write);

	VkDescriptorSetLayout layout;
	VkDescriptorPool pool;
	VkDescriptorSet set;

private:
	std::vector<VkDescriptorSetLayoutBinding> bindings{};
	std::vector<VkDescriptorPoolSize> pool_sizes{};
	std::vector<VkWriteDescriptorSet> writes{};

	ContextManagerSPtr context_manager_sptr;
};
