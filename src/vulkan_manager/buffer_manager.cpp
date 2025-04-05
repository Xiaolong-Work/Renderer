#include <buffer_manager.h>

BufferManager::BufferManager(const ContextManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager)
{
	this->pContentManager = pContentManager;
	this->pCommandManager = pCommandManager;
}

uint32_t BufferManager::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(pContentManager->physical_device, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type!");
}

void BufferManager::createBuffer(const VkDeviceSize size,
								 const VkBufferUsageFlags usage,
								 const VkMemoryPropertyFlags properties,
								 VkBuffer& buffer,
								 VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;
	bufferInfo.flags = 0;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.queueFamilyIndexCount = 0;
	bufferInfo.pQueueFamilyIndices = nullptr;

	if (vkCreateBuffer(pContentManager->device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(pContentManager->device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
	if (vkAllocateMemory(pContentManager->device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate buffer memory!");
	}

	vkBindBufferMemory(pContentManager->device, buffer, bufferMemory, 0);
}

void BufferManager::copyBuffer(const VkBuffer& srcBuffer, VkBuffer& dstBuffer, const VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = pCommandManager->beginTransferCommands();

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	pCommandManager->endTransferCommands(commandBuffer);
}

void BufferManager::createDeviceLocalBuffer(const VkDeviceSize size,
											const void* data,
											const VkBufferUsageFlags usage,
											VkBuffer& buffer,
											VkDeviceMemory& bufferMemory)
{
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(size,
				 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				 stagingBuffer,
				 stagingBufferMemory);

	void* tempData;
	vkMapMemory(pContentManager->device, stagingBufferMemory, 0, size, 0, &tempData);
	memcpy(tempData, data, (size_t)size);
	vkUnmapMemory(pContentManager->device, stagingBufferMemory);

	createBuffer(
		size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, bufferMemory);

	copyBuffer(stagingBuffer, buffer, size);

	vkDestroyBuffer(pContentManager->device, stagingBuffer, nullptr);
	vkFreeMemory(pContentManager->device, stagingBufferMemory, nullptr);
}

VertexBufferManager::VertexBufferManager(const ContextManagerSPtr& pContentManager,
										 const CommandManagerSPtr& pCommandManager)
{
	this->pContentManager = pContentManager;
	this->pCommandManager = pCommandManager;
}

void VertexBufferManager::init()
{
	VkDeviceSize bufferSize = sizeof(this->vertices[0]) * this->vertices.size();

	VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	if (this->enable_ray_tracing)
	{
		usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	}

	createDeviceLocalBuffer(bufferSize, this->vertices.data(), usage, this->buffer, this->memory);
}

void VertexBufferManager::clear()
{
	vkDestroyBuffer(pContentManager->device, this->buffer, nullptr);
	vkFreeMemory(pContentManager->device, this->memory, nullptr);
}

VkVertexInputBindingDescription VertexBufferManager::getBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> VertexBufferManager::getAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, normal);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, texture);

	return attributeDescriptions;
}

IndexBufferManager::IndexBufferManager(const ContextManagerSPtr& pContentManager,
									   const CommandManagerSPtr& pCommandManager)
{
	this->pContentManager = pContentManager;
	this->pCommandManager = pCommandManager;
}

void IndexBufferManager::init()
{
	VkDeviceSize bufferSize = sizeof(this->indices[0]) * this->indices.size();

	createDeviceLocalBuffer(
		bufferSize, this->indices.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, this->buffer, this->memory);
}

void IndexBufferManager::clear()
{
	vkDestroyBuffer(pContentManager->device, this->buffer, nullptr);
	vkFreeMemory(pContentManager->device, this->memory, nullptr);
}

IndirectBufferManager::IndirectBufferManager(const ContextManagerSPtr& pContentManager,
											 const CommandManagerSPtr& pCommandManager)
{
	this->pContentManager = pContentManager;
	this->pCommandManager = pCommandManager;
}

void IndirectBufferManager::init()
{
	VkDeviceSize bufferSize = sizeof(VkDrawIndexedIndirectCommand) * this->commands.size();

	createDeviceLocalBuffer(
		bufferSize, this->commands.data(), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, this->buffer, this->memory);
}

void IndirectBufferManager::clear()
{
	vkDestroyBuffer(pContentManager->device, this->buffer, nullptr);
	vkFreeMemory(pContentManager->device, this->memory, nullptr);
}

UniformBufferManager::UniformBufferManager(const ContextManagerSPtr& pContentManager,
										   const CommandManagerSPtr& pCommandManager)
{
	this->pContentManager = pContentManager;
	this->pCommandManager = pCommandManager;
}

void UniformBufferManager::init()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	createBuffer(bufferSize,
				 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				 this->uniformBuffer,
				 this->uniformBuffersMemory);

	vkMapMemory(pContentManager->device, this->uniformBuffersMemory, 0, bufferSize, 0, &this->uniformBuffersMapped);
}

void UniformBufferManager::clear()
{
	vkDestroyBuffer(pContentManager->device, this->uniformBuffer, nullptr);
	vkFreeMemory(pContentManager->device, this->uniformBuffersMemory, nullptr);
}

void UniformBufferManager::update(const UniformBufferObject& ubo)
{
	memcpy(this->uniformBuffersMapped, &ubo, sizeof(ubo));
}

StorageBufferManager::StorageBufferManager(const ContextManagerSPtr& pContentManager,
										   const CommandManagerSPtr& pCommandManager)
{
	this->pContentManager = pContentManager;
	this->pCommandManager = pCommandManager;
}

void StorageBufferManager::init()
{
	createDeviceLocalBuffer(this->size, this->data, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, this->buffer, this->memory);
}

void StorageBufferManager::clear()
{
	vkDestroyBuffer(pContentManager->device, this->buffer, nullptr);
	vkFreeMemory(pContentManager->device, this->memory, nullptr);
}

void StorageBufferManager::setData(const void* data, const VkDeviceSize size, const int length)
{
	this->data = data;
	this->size = size;
	this->length = length;
}

StagingBufferManager::StagingBufferManager(const ContextManagerSPtr& pContentManager,
										   const CommandManagerSPtr& pCommandManager)
{
	this->pContentManager = pContentManager;
	this->pCommandManager = pCommandManager;
}

void StagingBufferManager::init()
{
	createBuffer(this->size,
				 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				 this->buffer,
				 this->memory);

	vkMapMemory(pContentManager->device, this->memory, 0, this->size, 0, &this->mapped);
}

void StagingBufferManager::clear()
{
	vkDestroyBuffer(pContentManager->device, this->buffer, nullptr);
	vkFreeMemory(pContentManager->device, this->memory, nullptr);
}
