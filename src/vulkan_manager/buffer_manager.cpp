#include <buffer_manager.h>

BufferManager::BufferManager(const ContentManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager)
{
    this->pContentManager = pContentManager;
    this->pCommandManager = pCommandManager;
}

uint32_t BufferManager::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(pContentManager->physicalDevice, &memoryProperties);

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
    auto ret = vkAllocateMemory(pContentManager->device, &allocInfo, nullptr, &bufferMemory);
    if (ret != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }

    vkBindBufferMemory(pContentManager->device, buffer, bufferMemory, 0);
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

VertexBufferManager::VertexBufferManager(const ContentManagerSPtr& pContentManager,
                                         const CommandManagerSPtr& pCommandManager)
{
    this->pContentManager = pContentManager;
    this->pCommandManager = pCommandManager;
}

void VertexBufferManager::init()
{
    VkDeviceSize bufferSize = sizeof(this->vertices[0]) * this->vertices.size();

    createDeviceLocalBuffer(
        bufferSize, this->vertices.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, this->buffer, this->memory);
}

void VertexBufferManager::clear()
{
    vkDestroyBuffer(pContentManager->device, this->buffer, nullptr);
    vkFreeMemory(pContentManager->device, this->memory, nullptr);
}

IndexBufferManager::IndexBufferManager(const ContentManagerSPtr& pContentManager,
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

UniformBufferManager::UniformBufferManager(const ContentManagerSPtr& pContentManager,
                                           const CommandManagerSPtr& pCommandManager)
{
    this->pContentManager = pContentManager;
    this->pCommandManager = pCommandManager;
}

void UniformBufferManager::init()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    this->uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    this->uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    this->uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        createBuffer(bufferSize,
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     this->uniformBuffers[i],
                     this->uniformBuffersMemory[i]);

        vkMapMemory(
            pContentManager->device, this->uniformBuffersMemory[i], 0, bufferSize, 0, &this->uniformBuffersMapped[i]);
    }
}

void UniformBufferManager::clear()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(pContentManager->device, this->uniformBuffers[i], nullptr);
        vkFreeMemory(pContentManager->device, this->uniformBuffersMemory[i], nullptr);
    }
}

void UniformBufferManager::update(const UniformBufferObject& ubo, const int index)
{
    memcpy(this->uniformBuffersMapped[index], &ubo, sizeof(ubo));
}

StorageBufferManager::StorageBufferManager(const ContentManagerSPtr& pContentManager,
                                           const CommandManagerSPtr& pCommandManager)
{
    this->pContentManager = pContentManager;
    this->pCommandManager = pCommandManager;
}

void StorageBufferManager::init() { }

void StorageBufferManager::clear()
{
    vkDestroyBuffer(pContentManager->device, this->buffer, nullptr);
    vkFreeMemory(pContentManager->device, this->memory, nullptr);
}
