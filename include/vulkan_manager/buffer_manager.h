#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <command_manager.h>
#include <content_manager.h>

#include <array>
#include <chrono>
#include <memory>
#include <stdexcept>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class BufferManager
{
public:
    BufferManager() = default;
    BufferManager(const ContentManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager);

    virtual void init() = 0;
    virtual void clear() = 0;

protected:
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void createBuffer(const VkDeviceSize size,
                      const VkBufferUsageFlags usage,
                      const VkMemoryPropertyFlags properties,
                      VkBuffer& buffer,
                      VkDeviceMemory& bufferMemory);

    void createDeviceLocalBuffer(const VkDeviceSize size,
                                 const void* data,
                                 const VkBufferUsageFlags usage,
                                 VkBuffer& buffer,
                                 VkDeviceMemory& bufferMemory);

    void copyBuffer(const VkBuffer& srcBuffer, VkBuffer& dstBuffer, const VkDeviceSize size);

    ContentManagerSPtr pContentManager;
    CommandManagerSPtr pCommandManager;
};

typedef std::shared_ptr<BufferManager> BufferManagerSPtr;

struct Vertex1
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex1);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex1, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex1, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex1, texCoord);

        return attributeDescriptions;
    }
};

class VertexBufferManager : public BufferManager
{
public:
    VertexBufferManager() = default;
    VertexBufferManager(const ContentManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager);

    void init() override;
    void clear() override;

    VkBuffer buffer;
    VkDeviceMemory memory;
    std::vector<Vertex1> vertices;
};

class IndexBufferManager : public BufferManager
{
public:
    IndexBufferManager() = default;
    IndexBufferManager(const ContentManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager);

    void init() override;
    void clear() override;

    VkBuffer buffer;
    VkDeviceMemory memory;
    std::vector<uint32_t> indices;
};

struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class UniformBufferManager : public BufferManager
{
public:
    UniformBufferManager() = default;
    UniformBufferManager(const ContentManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager);

    void init() override;
    void clear() override;

    void update(const UniformBufferObject& ubo, const int index);

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
};

typedef std::shared_ptr<UniformBufferManager> UniformBufferManagerSPtr;

class StorageBufferManager : public BufferManager
{
public:
    StorageBufferManager() = default;
    StorageBufferManager(const ContentManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager);

    void init() override;
    void clear() override;

    VkBuffer buffer;
    VkDeviceMemory memory;
};
