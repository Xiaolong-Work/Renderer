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

struct alignas(16) SSBOMaterial
{
	/* ========== Blinn-Phong ========== */
	/* Ambient reflectance component (ka). */
	glm::vec3 ka{0.0f};

	/* Shininess coefficient (glossiness factor). */
	float ns{0.0f};

	/* Diffuse reflectance component (kd). */
	glm::vec3 kd{0.0f};

	/* The diffuse texture index of the object, -1 means no texture data */
	Index diffuse_texture{-1};

	/* Specular reflectance component (ks). */
	glm::vec3 ks{0.0f};

	/* The specular texture index of the object, -1 means no texture data */
	Index specular_texture{-1};

	/* Transmittance component (tr). */
	glm::vec3 tr{0.0f};

	/* Refractive index of the material (ni). */
	float ni{0.0f};

	/* ========== PBR ========== */
	glm::vec4 albedo;
	int albedo_texture;
	float metallic;
	float roughness;

	float pad{0};

	glm::vec4 bottom_albedo;
	int bottom_albedo_texture{-1};
	float bottom_metallic;
	float bottom_roughness;

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

		this->bottom_albedo = material.bottom_albedo;
		this->bottom_metallic = material.bottom_metallic;
		this->bottom_roughness = material.bottom_roughness;
		this->bottom_albedo_texture = material.bottom_albedo_texture;

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

	VkDeviceAddress getBufferAddress();

	VkBuffer buffer{};
	VkDeviceMemory memory{};

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

	std::vector<Vertex> vertices{};
	std::vector<uint32_t> vertex_count{};

	VkBufferUsageFlags usage{VK_BUFFER_USAGE_VERTEX_BUFFER_BIT};
};

typedef std::shared_ptr<VertexBufferManager> VertexBufferManagerSPtr;

class IndexBufferManager : public BufferManager
{
public:
	IndexBufferManager() = default;
	IndexBufferManager(const ContextManagerSPtr& context_manager_sptr, const CommandManagerSPtr& command_manager_sptr);

	void init() override;
	void clear() override;

	std::vector<Index> indices{};
	bool enable_ray_tracing{false};
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
	std::pair<VkDescriptorSetLayoutBinding, VkWriteDescriptorSet> getDescriptor(const uint32_t binding,
																				const VkShaderStageFlags flag);

private:
	const void* data{nullptr};
	VkDeviceSize size{0};
	uint32_t length{0};

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
	std::pair<VkDescriptorSetLayoutBinding, VkWriteDescriptorSet> getDescriptor(const uint32_t binding,
																				const VkShaderStageFlags flag);

	VkBufferUsageFlags usage{VK_BUFFER_USAGE_STORAGE_BUFFER_BIT};
	bool enable_ray_tracing{false};

private:
	const void* data{nullptr};
	VkDeviceSize size{0};
	uint32_t length{0};

	VkDescriptorBufferInfo descriptor_buffer{};
};

typedef std::shared_ptr<StorageBufferManager> StorageBufferManagerSPtr;

class RandomBufferManager : public StorageBufferManager
{
public:
	RandomBufferManager() = default;
	RandomBufferManager(const ContextManagerSPtr& context_manager_sptr, const CommandManagerSPtr& command_manager_sptr);

	void init() override;
	void clear() override;

	void setSize(const int size);

private:
	int size{0};
};

class StagingBufferManager : public BufferManager
{
public:
	StagingBufferManager() = default;
	StagingBufferManager(const ContextManagerSPtr& context_manager_sptr,
						 const CommandManagerSPtr& command_manager_sptr);

	void init() override;
	void clear() override;

	void* mapped{nullptr};

	VkBufferUsageFlags usage{VK_BUFFER_USAGE_TRANSFER_SRC_BIT};
	VkDeviceSize size{0};
};
