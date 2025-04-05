#pragma once

#include <command_manager.h>
#include <context_manager.h>

#include <array>
#include <chrono>
#include <memory>
#include <stdexcept>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vertex.h>
#include <material.h>

struct SSBOMaterial
{
	/* Ambient reflectance component (ka). */
	alignas(16) glm::vec3 ka{0.0f};

	/* Diffuse reflectance component (kd). */
	alignas(16) glm::vec3 kd{0.0f};

	/* Specular reflectance component (ks). */
	alignas(16) glm::vec3 ks{0.0f};

	/* Transmittance component (tr). */
	alignas(16) glm::vec3 tr{0.0f};

	/* Shininess coefficient (glossiness factor). */
	float ns{0.0f};

	/* Refractive index of the material (ni). */
	float ni{0.0f};

	/* The diffuse texture index of the object, -1 means no texture data */
	int32_t diffuse_texture{-1};

	int type = 0;

	SSBOMaterial(const Material& material)
	{
		this->ka = material.ka;
		this->kd = material.kd;
		this->ks = material.ks;
		this->tr = material.tr;
		this->ns = material.ns;
		this->ni = material.ni;
		this->diffuse_texture = material.diffuse_texture;
	}
};

class BufferManager
{
public:
	BufferManager() = default;
	BufferManager(const ContextManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager);

	virtual void init() = 0;
	virtual void clear() = 0;

protected:
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	void createBuffer(const VkDeviceSize size,
					  const VkBufferUsageFlags usage,
					  const VkMemoryPropertyFlags properties,
					  VkBuffer& buffer,
					  VkDeviceMemory& bufferMemory);

	void copyBuffer(const VkBuffer& srcBuffer, VkBuffer& dstBuffer, const VkDeviceSize size);

	void createDeviceLocalBuffer(const VkDeviceSize size,
								 const void* data,
								 const VkBufferUsageFlags usage,
								 VkBuffer& buffer,
								 VkDeviceMemory& bufferMemory);

	ContextManagerSPtr pContentManager;
	CommandManagerSPtr pCommandManager;
};

typedef std::shared_ptr<BufferManager> BufferManagerSPtr;

class VertexBufferManager : public BufferManager
{
public:
	VertexBufferManager() = default;
	VertexBufferManager(const ContextManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager);

	void init() override;
	void clear() override;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();

	VkBuffer buffer;
	VkDeviceMemory memory;
	std::vector<Vertex> vertices;

	bool enable_ray_tracing{false};
};

class IndexBufferManager : public BufferManager
{
public:
	IndexBufferManager() = default;
	IndexBufferManager(const ContextManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager);

	void init() override;
	void clear() override;

	VkBuffer buffer;
	VkDeviceMemory memory;
	std::vector<int32_t> indices;
};

class IndirectBufferManager : public BufferManager
{
public:
	IndirectBufferManager() = default;
	IndirectBufferManager(const ContextManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager);

	void init() override;
	void clear() override;

	VkBuffer buffer;
	VkDeviceMemory memory;
	std::vector<VkDrawIndexedIndirectCommand> commands{};
};

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 project;
};

class UniformBufferManager : public BufferManager
{
public:
	UniformBufferManager() = default;
	UniformBufferManager(const ContextManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager);

	void init() override;
	void clear() override;

	void update(const UniformBufferObject& ubo);

	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBuffersMemory;
	void* uniformBuffersMapped;
};

class StorageBufferManager : public BufferManager
{
public:
	StorageBufferManager() = default;
	StorageBufferManager(const ContextManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager);

	void init() override;
	void clear() override;

	void setData(const void* data, const VkDeviceSize size, const int length);

	VkBuffer buffer;
	VkDeviceMemory memory;

	VkDeviceSize size;
	int length;

private:
	const void* data;
	
};

class StagingBufferManager : public BufferManager
{
public:
	StagingBufferManager() = default;
	StagingBufferManager(const ContextManagerSPtr& pContentManager, const CommandManagerSPtr& pCommandManager);

	void init() override;
	void clear() override;

	VkBuffer buffer{};
	VkDeviceMemory memory{};
	void* mapped = nullptr;

	VkDeviceSize size{0};
};
