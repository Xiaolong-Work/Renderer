#include <descriptor_manager.h>

DescriptorManager::DescriptorManager(const ContextManagerSPtr& pContentManager)
{
	this->pContentManager = pContentManager;
}

void DescriptorManager::init()
{
	createDescriptorSetLayout();
	createDescriptorPool();
	createDescriptorSet();
}

void DescriptorManager::clear()
{
	vkDestroyDescriptorPool(pContentManager->device, pool, nullptr);
	vkDestroyDescriptorSetLayout(pContentManager->device, layout, nullptr);
}

void DescriptorManager::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pNext = nullptr;
	layoutInfo.flags = 0;
	layoutInfo.bindingCount = static_cast<uint32_t>(this->bindings.size());
	layoutInfo.pBindings = this->bindings.data();

	if (vkCreateDescriptorSetLayout(pContentManager->device, &layoutInfo, nullptr, &this->layout) != VK_SUCCESS)
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
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();

	if (vkCreateDescriptorPool(pContentManager->device, &poolInfo, nullptr, &this->pool) != VK_SUCCESS)
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

	if (vkAllocateDescriptorSets(pContentManager->device, &allocInfo, &this->set) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}
}
