#pragma once

#include <array>
#include <map>
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

	void addLayoutBinding(const VkDescriptorSetLayoutBinding& binding);
	void addPoolSize(const VkDescriptorType type, const uint32_t size);
	void addWrite(const VkWriteDescriptorSet& write);

	void addDescriptor(const std::pair<VkDescriptorSetLayoutBinding, VkWriteDescriptorSet>& descriptor);
	void addDescriptors(const std::vector<std::pair<VkDescriptorSetLayoutBinding, VkWriteDescriptorSet>>& descriptors);

		VkDescriptorSetLayout layout { };
	VkDescriptorPool pool{};
	VkDescriptorSet set{};

protected:
	void createDescriptorSetLayout();
	void createDescriptorPool();
	void createDescriptorSet();

private:
	std::vector<VkDescriptorSetLayoutBinding> bindings{};
	std::map<VkDescriptorType, uint32_t> pool_sizes;
	std::vector<VkWriteDescriptorSet> writes{};

	ContextManagerSPtr context_manager_sptr;
};
