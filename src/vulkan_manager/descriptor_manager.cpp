#include <descriptor_manager.h>

DescriptorManager::DescriptorManager(const ContextManagerSPtr& context_manager_sptr)
{
	this->context_manager_sptr = context_manager_sptr;
}

void DescriptorManager::init()
{
	createDescriptorSetLayout();
	createDescriptorPool();
	createDescriptorSet();
}

void DescriptorManager::clear()
{
	vkDestroyDescriptorPool(context_manager_sptr->device, pool, nullptr);
	vkDestroyDescriptorSetLayout(context_manager_sptr->device, layout, nullptr);
}

void DescriptorManager::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = nullptr;
	layoutInfo.flags = 0;
	layoutInfo.bindingCount = static_cast<uint32_t>(this->bindings.size());
	layoutInfo.pBindings = this->bindings.data();

	if (vkCreateDescriptorSetLayout(context_manager_sptr->device, &layoutInfo, nullptr, &this->layout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout!");
	}
}

void DescriptorManager::createDescriptorPool()
{
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.pNext = nullptr;
	poolInfo.flags = 0;
	poolInfo.maxSets = static_cast<uint32_t>(1);
	poolInfo.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	poolInfo.pPoolSizes = pool_sizes.data();

	if (vkCreateDescriptorPool(context_manager_sptr->device, &poolInfo, nullptr, &this->pool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool!");
	}
}

void DescriptorManager::createDescriptorSet()
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = this->pool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(1);
	allocInfo.pSetLayouts = &this->layout;

	if (vkAllocateDescriptorSets(context_manager_sptr->device, &allocInfo, &this->set) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}

	for (auto& write : this->writes)
	{
		write.dstSet = this->set;
	}
	vkUpdateDescriptorSets(
		context_manager_sptr->device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void DescriptorManager::addLayoutBinding(const VkDescriptorSetLayoutBinding& binding)
{
	this->bindings.push_back(binding);
}

void DescriptorManager::addPoolSize(const VkDescriptorType type, const uint32_t size)
{
	VkDescriptorPoolSize pool_size{};
	pool_size.type = type;
	pool_size.descriptorCount = size;
	this->pool_sizes.push_back(pool_size);
}

void DescriptorManager::addWrite(const VkWriteDescriptorSet& write)
{
	this->writes.push_back(write);
}
