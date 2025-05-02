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
	std::vector<VkDescriptorPoolSize> all_pool_sizes{};

	for (auto& iter : this->pool_sizes)
	{
		VkDescriptorPoolSize pool_size{};
		pool_size.type = iter.first;
		pool_size.descriptorCount = iter.second;
		all_pool_sizes.push_back(pool_size);
	}

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.pNext = nullptr;
	poolInfo.flags = 0;
	poolInfo.maxSets = 1;
	poolInfo.poolSizeCount = static_cast<uint32_t>(all_pool_sizes.size());
	poolInfo.pPoolSizes = all_pool_sizes.data();

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
	allocInfo.descriptorSetCount = 1;
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
		context_manager_sptr->device, static_cast<uint32_t>(this->writes.size()), this->writes.data(), 0, nullptr);
}

void DescriptorManager::addDescriptor(const std::pair<VkDescriptorSetLayoutBinding, VkWriteDescriptorSet>& descriptor)
{
	auto& binding = descriptor.first;

	this->bindings.push_back(binding);

	if (this->pool_sizes.find(binding.descriptorType) != this->pool_sizes.end())
	{
		this->pool_sizes[binding.descriptorType]++;
	}
	else
	{
		this->pool_sizes[binding.descriptorType] = 1;
	}

	this->writes.push_back(descriptor.second);
}

void DescriptorManager::addDescriptors(
	const std::vector<std::pair<VkDescriptorSetLayoutBinding, VkWriteDescriptorSet>>& descriptors)
{
	for (auto& descriptor : descriptors)
	{
		addDescriptor(descriptor);
	}
}

void DescriptorManager::addLayoutBinding(const VkDescriptorSetLayoutBinding& binding)
{
	this->bindings.push_back(binding);

	if (this->pool_sizes.find(binding.descriptorType) != this->pool_sizes.end())
	{
		this->pool_sizes[binding.descriptorType]++;
	}
	else
	{
		this->pool_sizes[binding.descriptorType] = 1;
	}
}

void DescriptorManager::addPoolSize(const VkDescriptorType type, const uint32_t size)
{
	this->pool_sizes[type] = size;
}

void DescriptorManager::addWrite(const VkWriteDescriptorSet& write)
{
	this->writes.push_back(write);
}
