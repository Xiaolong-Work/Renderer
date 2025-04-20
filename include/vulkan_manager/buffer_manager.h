#pragma once

#include <command_manager.h>
#include <context_manager.h>

#include <array>
#include <chrono>
#include <memory>
#include <stdexcept>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <material.h>
#include <vertex.h>

struct SSBOMaterial
{
	/* ========== Blinn-Phong ========== */
	/* Ambient reflectance component (ka). */
	alignas(16) glm::vec3 ka{0.0f};

	/* Shininess coefficient (glossiness factor). */
	float ns{0.0f};

	/* Diffuse reflectance component (kd). */
	alignas(16) glm::vec3 kd{0.0f};

	/* The diffuse texture index of the object, -1 means no texture data */
	Index diffuse_texture{-1};

	/* Specular reflectance component (ks). */
	alignas(16) glm::vec3 ks{0.0f};

	/* The specular texture index of the object, -1 means no texture data */
	Index specular_texture{-1};

	/* Transmittance component (tr). */
	alignas(16) glm::vec3 tr{0.0f};

	/* Refractive index of the material (ni). */
	float ni{0.0f};

	/* ========== PBR ========== */
	alignas(16) Vector4f albedo;
	int albedo_texture;
	float metallic;
	float roughness;

	int type{0};

	SSBOMaterial(const Material& material)
	{
		this->ka = material.ka;
		this->kd = material.kd;
		this->ks = material.ks;
		this->tr = material.tr;
		this->ns = material.ns;
		this->ni = material.ni;
		this->diffuse_texture = material.diffuse_texture;
		this->specular_texture = material.specular_texture;

		this->albedo = material.albedo;
		this->metallic = material.metallic;
		this->roughness = material.roughness;
		this->albedo_texture = material.albedo_texture;

		this->type = int(material.type);
	}
};

struct UBOMVP
{
	alignas(16) glm::mat4 model{};
	alignas(16) glm::mat4 view{};
	alignas(16) glm::mat4 project{};
};

class BufferManager
{
public:
	BufferManager() = default;
	BufferManager(const ContextManagerSPtr& context_manager_sptr, const CommandManagerSPtr& command_manager_sptr);

	virtual void init() = 0;
	virtual void clear() = 0;

protected:
	uint32_t findMemoryType(const uint32_t filter, const VkMemoryPropertyFlags properties) const;

	void createBuffer(const VkDeviceSize size,
					  const VkBufferUsageFlags usage,
					  const VkMemoryPropertyFlags properties,
					  VkBuffer& buffer,
					  VkDeviceMemory& memory) const;

	void copyBuffer(const VkBuffer& source, VkBuffer& destination, const VkDeviceSize size) const;

	void createDeviceLocalBuffer(const VkDeviceSize size,
								 const void* data,
								 const VkBufferUsageFlags usage,
								 VkBuffer& buffer,
								 VkDeviceMemory& memory) const;

	ContextManagerSPtr context_manager_sptr{};
	CommandManagerSPtr command_manager_sptr{};
};

typedef std::shared_ptr<BufferManager> BufferManagerSPtr;

class VertexBufferManager : public BufferManager
{
public:
	VertexBufferManager() = default;
	VertexBufferManager(const ContextManagerSPtr& context_manager_sptr, const CommandManagerSPtr& command_manager_sptr);

	void init() override;
	void clear() override;

	VkBuffer buffer{};
	VkDeviceMemory memory{};
	std::vector<Vertex> vertices{};

	bool enable_ray_tracing{false};
};

typedef std::shared_ptr<VertexBufferManager> VertexBufferManagerSPtr;

class IndexBufferManager : public BufferManager
{
public:
	IndexBufferManager() = default;
	IndexBufferManager(const ContextManagerSPtr& context_manager_sptr, const CommandManagerSPtr& command_manager_sptr);

	void init() override;
	void clear() override;

	VkBuffer buffer{};
	VkDeviceMemory memory{};
	std::vector<Index> indices{};
};

typedef std::shared_ptr<IndexBufferManager> IndexBufferManagerSPtr;

class IndirectBufferManager : public BufferManager
{
public:
	IndirectBufferManager() = default;
	IndirectBufferManager(const ContextManagerSPtr& context_manager_sptr,
						  const CommandManagerSPtr& command_manager_sptr);

	void init() override;
	void clear() override;

	VkBuffer buffer{};
	VkDeviceMemory memory{};
	std::vector<VkDrawIndexedIndirectCommand> commands{};
};

typedef std::shared_ptr<IndirectBufferManager> IndirectBufferManagerSPtr;

class UniformBufferManager : public BufferManager
{
public:
	UniformBufferManager() = default;
	UniformBufferManager(const ContextManagerSPtr& context_manager_sptr,
						 const CommandManagerSPtr& command_manager_sptr);

	void init() override;
	void clear() override;

	void setData(const void* data, const VkDeviceSize size, const uint32_t length);

	void update(const void* data);

	VkDescriptorSetLayoutBinding getLayoutBinding(const uint32_t binding, const VkShaderStageFlags flag);
	VkWriteDescriptorSet getWriteInformation(const uint32_t binding);

private:
	const void* data{nullptr};
	VkDeviceSize size{0};
	uint32_t length{0};

	VkBuffer buffer{};
	VkDeviceMemory memory{};
	VkDescriptorBufferInfo descriptor_buffer{};
	void* mapped{nullptr};
};

class StorageBufferManager : public BufferManager
{
public:
	StorageBufferManager() = default;
	StorageBufferManager(const ContextManagerSPtr& context_manager_sptr,
						 const CommandManagerSPtr& command_manager_sptr);

	void init() override;
	void clear() override;

	void setData(const void* data, const VkDeviceSize size, const uint32_t length);

	VkDescriptorSetLayoutBinding getLayoutBinding(const uint32_t binding, const VkShaderStageFlags flag);
	VkWriteDescriptorSet getWriteInformation(const uint32_t binding);

private:
	const void* data{nullptr};
	VkDeviceSize size{0};
	uint32_t length{0};

	VkBuffer buffer{};
	VkDeviceMemory memory{};
	VkDescriptorBufferInfo descriptor_buffer{};
};

typedef std::shared_ptr<StorageBufferManager> StorageBufferManagerSPtr;

class StagingBufferManager : public BufferManager
{
public:
	StagingBufferManager() = default;
	StagingBufferManager(const ContextManagerSPtr& context_manager_sptr,
						 const CommandManagerSPtr& command_manager_sptr);

	void init() override;
	void clear() override;

	VkBuffer buffer{};
	VkDeviceMemory memory{};
	void* mapped{nullptr};

	VkDeviceSize size{0};
};
