#include <buffer_manager.h>

BufferManager::BufferManager(const ContextManagerSPtr& context_manager_sptr,
							 const CommandManagerSPtr& command_manager_sptr)
{
	this->context_manager_sptr = context_manager_sptr;
	this->command_manager_sptr = command_manager_sptr;
}

VkDeviceAddress BufferManager::getBufferAddress()
{
	VkBufferDeviceAddressInfo address{};
	address.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	address.pNext = nullptr;
	address.buffer = this->buffer;

	return vkGetBufferDeviceAddress(this->context_manager_sptr->device, &address);
}

uint32_t BufferManager::findMemoryType(const uint32_t filter, const VkMemoryPropertyFlags properties) const
{
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(context_manager_sptr->physical_device, &memory_properties);

	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
	{
		if ((filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
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
								 VkDeviceMemory& memory) const
{
	VkBufferCreateInfo buffer_create_information{};
	buffer_create_information.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_information.pNext = nullptr;
	buffer_create_information.flags = 0;
	buffer_create_information.size = size;
	buffer_create_information.usage = usage;
	buffer_create_information.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_create_information.queueFamilyIndexCount = 0;
	buffer_create_information.pQueueFamilyIndices = nullptr;

	if (vkCreateBuffer(context_manager_sptr->device, &buffer_create_information, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create buffer!");
	}

	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(context_manager_sptr->device, buffer, &requirements);

	VkMemoryAllocateInfo allocate_information{};
	allocate_information.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_information.pNext = nullptr;
	allocate_information.allocationSize = requirements.size;
	allocate_information.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, properties);
	if (vkAllocateMemory(context_manager_sptr->device, &allocate_information, nullptr, &memory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate buffer memory!");
	}

	vkBindBufferMemory(context_manager_sptr->device, buffer, memory, 0);
}

void BufferManager::copyBuffer(const VkBuffer& source, VkBuffer& destination, const VkDeviceSize size) const
{
	VkCommandBuffer command_buffer = command_manager_sptr->beginTransferCommands();

	VkBufferCopy copy_region{};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = 0;
	copy_region.size = size;
	vkCmdCopyBuffer(command_buffer, source, destination, 1, &copy_region);

	command_manager_sptr->endTransferCommands(command_buffer);
}

void BufferManager::createDeviceLocalBuffer(const VkDeviceSize size,
											const void* data,
											const VkBufferUsageFlags usage,
											VkBuffer& buffer,
											VkDeviceMemory& memory) const
{
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	createBuffer(size,
				 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				 staging_buffer,
				 staging_buffer_memory);

	void* temp_data;
	vkMapMemory(context_manager_sptr->device, staging_buffer_memory, 0, size, 0, &temp_data);
	memcpy(temp_data, data, (size_t)size);
	vkUnmapMemory(context_manager_sptr->device, staging_buffer_memory);

	createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, memory);

	copyBuffer(staging_buffer, buffer, size);

	vkDestroyBuffer(context_manager_sptr->device, staging_buffer, nullptr);
	vkFreeMemory(context_manager_sptr->device, staging_buffer_memory, nullptr);
}

VertexBufferManager::VertexBufferManager(const ContextManagerSPtr& context_manager_sptr,
										 const CommandManagerSPtr& command_manager_sptr)
{
	this->context_manager_sptr = context_manager_sptr;
	this->command_manager_sptr = command_manager_sptr;
}

void VertexBufferManager::init()
{
	VkDeviceSize size = sizeof(this->vertices[0]) * this->vertices.size();

	VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	if (this->enable_ray_tracing)
	{
		usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	}

	createDeviceLocalBuffer(size, this->vertices.data(), usage, this->buffer, this->memory);
}

void VertexBufferManager::clear()
{
	vkDestroyBuffer(context_manager_sptr->device, this->buffer, nullptr);
	vkFreeMemory(context_manager_sptr->device, this->memory, nullptr);
}

IndexBufferManager::IndexBufferManager(const ContextManagerSPtr& context_manager_sptr,
									   const CommandManagerSPtr& command_manager_sptr)
{
	this->context_manager_sptr = context_manager_sptr;
	this->command_manager_sptr = command_manager_sptr;
}

void IndexBufferManager::init()
{
	VkDeviceSize size = sizeof(this->indices[0]) * this->indices.size();

	createDeviceLocalBuffer(size, this->indices.data(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, this->buffer, this->memory);
}

void IndexBufferManager::clear()
{
	vkDestroyBuffer(context_manager_sptr->device, this->buffer, nullptr);
	vkFreeMemory(context_manager_sptr->device, this->memory, nullptr);
}

IndirectBufferManager::IndirectBufferManager(const ContextManagerSPtr& context_manager_sptr,
											 const CommandManagerSPtr& command_manager_sptr)
{
	this->context_manager_sptr = context_manager_sptr;
	this->command_manager_sptr = command_manager_sptr;
}

void IndirectBufferManager::init()
{
	VkDeviceSize size = sizeof(VkDrawIndexedIndirectCommand) * this->commands.size();

	createDeviceLocalBuffer(
		size, this->commands.data(), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, this->buffer, this->memory);
}

void IndirectBufferManager::clear()
{
	vkDestroyBuffer(context_manager_sptr->device, this->buffer, nullptr);
	vkFreeMemory(context_manager_sptr->device, this->memory, nullptr);
}

UniformBufferManager::UniformBufferManager(const ContextManagerSPtr& context_manager_sptr,
										   const CommandManagerSPtr& command_manager_sptr)
{
	this->context_manager_sptr = context_manager_sptr;
	this->command_manager_sptr = command_manager_sptr;
}

void UniformBufferManager::init()
{
	createBuffer(this->size,
				 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				 this->buffer,
				 this->memory);

	vkMapMemory(context_manager_sptr->device, this->memory, 0, this->size, 0, &this->mapped);
}

void UniformBufferManager::clear()
{
	vkDestroyBuffer(context_manager_sptr->device, this->buffer, nullptr);
	vkFreeMemory(context_manager_sptr->device, this->memory, nullptr);
}

void UniformBufferManager::update(const void* data)
{
	memcpy(this->mapped, data, this->size);
}

void UniformBufferManager::setData(const void* data, const VkDeviceSize size, const uint32_t length)
{
	this->data = data;
	this->size = size;
	this->length = length;
}

VkDescriptorSetLayoutBinding UniformBufferManager::getLayoutBinding(const uint32_t binding,
																	const VkShaderStageFlags flag)
{
	VkDescriptorSetLayoutBinding layout_binding{};
	layout_binding.binding = binding;
	layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layout_binding.descriptorCount = this->length;
	layout_binding.stageFlags = flag;
	layout_binding.pImmutableSamplers = nullptr;

	return layout_binding;
}

VkWriteDescriptorSet UniformBufferManager::getWriteInformation(const uint32_t binding)
{
	this->descriptor_buffer.buffer = this->buffer;
	this->descriptor_buffer.offset = 0;
	this->descriptor_buffer.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet buffer_write{};
	buffer_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	buffer_write.pNext = nullptr;
	buffer_write.dstSet = VK_NULL_HANDLE;
	buffer_write.dstBinding = binding;
	buffer_write.dstArrayElement = 0;
	buffer_write.descriptorCount = 1;
	buffer_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	buffer_write.pBufferInfo = &this->descriptor_buffer;
	buffer_write.pImageInfo = nullptr;
	buffer_write.pTexelBufferView = nullptr;

	return buffer_write;
}

StorageBufferManager::StorageBufferManager(const ContextManagerSPtr& context_manager_sptr,
										   const CommandManagerSPtr& command_manager_sptr)
{
	this->context_manager_sptr = context_manager_sptr;
	this->command_manager_sptr = command_manager_sptr;
}

void StorageBufferManager::init()
{
	createDeviceLocalBuffer(this->size, this->data, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, this->buffer, this->memory);
}

void StorageBufferManager::clear()
{
	vkDestroyBuffer(context_manager_sptr->device, this->buffer, nullptr);
	vkFreeMemory(context_manager_sptr->device, this->memory, nullptr);
}

void StorageBufferManager::setData(const void* data, const VkDeviceSize size, const uint32_t length)
{
	this->data = data;
	this->size = size;
	this->length = length;
}

VkDescriptorSetLayoutBinding StorageBufferManager::getLayoutBinding(const uint32_t binding,
																	const VkShaderStageFlags flag)
{
	VkDescriptorSetLayoutBinding layout_binding{};
	layout_binding.binding = binding;
	layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	layout_binding.descriptorCount = this->length;
	layout_binding.stageFlags = flag;
	layout_binding.pImmutableSamplers = nullptr;

	return layout_binding;
}

VkWriteDescriptorSet StorageBufferManager::getWriteInformation(const uint32_t binding)
{
	this->descriptor_buffer.buffer = this->buffer;
	this->descriptor_buffer.offset = 0;
	this->descriptor_buffer.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet buffer_write{};
	buffer_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	buffer_write.pNext = nullptr;
	buffer_write.dstSet = VK_NULL_HANDLE;
	buffer_write.dstBinding = binding;
	buffer_write.dstArrayElement = 0;
	buffer_write.descriptorCount = 1;
	buffer_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	buffer_write.pBufferInfo = &this->descriptor_buffer;
	buffer_write.pImageInfo = nullptr;
	buffer_write.pTexelBufferView = nullptr;

	return buffer_write;
}

StagingBufferManager::StagingBufferManager(const ContextManagerSPtr& context_manager_sptr,
										   const CommandManagerSPtr& command_manager_sptr)
{
	this->context_manager_sptr = context_manager_sptr;
	this->command_manager_sptr = command_manager_sptr;
}

void StagingBufferManager::init()
{
	createBuffer(this->size,
				 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				 this->buffer,
				 this->memory);

	vkMapMemory(context_manager_sptr->device, this->memory, 0, this->size, 0, &this->mapped);
}

void StagingBufferManager::clear()
{
	vkDestroyBuffer(context_manager_sptr->device, this->buffer, nullptr);
	vkFreeMemory(context_manager_sptr->device, this->memory, nullptr);
}
