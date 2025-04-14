#include <command_manager.h>

CommandManager::CommandManager(const ContextManagerSPtr& context_manager_sptr)
{
	this->context_manager_sptr = context_manager_sptr;
}

void CommandManager::init()
{
	createCommandPool();
	createCommandBuffers();
}

void CommandManager::clear()
{
	vkDestroyCommandPool(context_manager_sptr->device, this->graphicsCommandPool, nullptr);
	vkDestroyCommandPool(context_manager_sptr->device, this->transferCommandPool, nullptr);
	vkDestroyCommandPool(context_manager_sptr->device, this->computeCommandPool, nullptr);
}

void CommandManager::createCommandPool()
{
	VkCommandPoolCreateInfo graphicsPoolInfo{};
	graphicsPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	graphicsPoolInfo.pNext = nullptr;
	graphicsPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	graphicsPoolInfo.queueFamilyIndex = context_manager_sptr->graphicsFamily;

	VkCommandPoolCreateInfo transferPoolInfo{};
	transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transferPoolInfo.pNext = nullptr;
	transferPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	transferPoolInfo.queueFamilyIndex = context_manager_sptr->transferFamily;

	VkCommandPoolCreateInfo computePoolInfo{};
	computePoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	computePoolInfo.pNext = nullptr;
	computePoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	computePoolInfo.queueFamilyIndex = context_manager_sptr->computeFamily;

	if (vkCreateCommandPool(context_manager_sptr->device, &graphicsPoolInfo, nullptr, &this->graphicsCommandPool) !=
		VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics command pool!");
	}

	if (vkCreateCommandPool(context_manager_sptr->device, &transferPoolInfo, nullptr, &this->transferCommandPool) !=
		VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create transfer command pool!");
	}

	if (vkCreateCommandPool(context_manager_sptr->device, &computePoolInfo, nullptr, &this->computeCommandPool) !=
		VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create compute command pool!");
	}
}

void CommandManager::createCommandBuffers()
{
	graphicsCommandBuffers.resize(2);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.commandPool = graphicsCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)graphicsCommandBuffers.size();

	if (vkAllocateCommandBuffers(context_manager_sptr->device, &allocInfo, graphicsCommandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate command buffers!");
	}
}

VkCommandBuffer CommandManager::beginTransferCommands()
{
	/* Creating a temporary command buffer */
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.commandPool = this->transferCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(context_manager_sptr->device, &allocInfo, &command_buffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;

	vkBeginCommandBuffer(command_buffer, &beginInfo);
	return command_buffer;
}

void CommandManager::endTransferCommands(VkCommandBuffer command_buffer)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit{};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;
	submit.waitSemaphoreCount = 0;
	submit.pWaitSemaphores = nullptr;
	submit.pWaitDstStageMask = nullptr;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &command_buffer;
	submit.signalSemaphoreCount = 0;
	submit.pSignalSemaphores = nullptr;

	vkQueueSubmit(context_manager_sptr->transferQueue, 1, &submit, VK_NULL_HANDLE);
	vkQueueWaitIdle(context_manager_sptr->transferQueue);
	vkFreeCommandBuffers(context_manager_sptr->device, this->transferCommandPool, 1, &command_buffer);
}

VkCommandBuffer CommandManager::beginComputeCommands()
{
	/* Creating a temporary command buffer */
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.commandPool = this->computeCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(context_manager_sptr->device, &allocInfo, &command_buffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;

	vkBeginCommandBuffer(command_buffer, &beginInfo);
	return command_buffer;
}

void CommandManager::endComputeCommands(VkCommandBuffer command_buffer)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit{};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;
	submit.waitSemaphoreCount = 0;
	submit.pWaitSemaphores = nullptr;
	submit.pWaitDstStageMask = nullptr;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &command_buffer;
	submit.signalSemaphoreCount = 0;
	submit.pSignalSemaphores = nullptr;

	vkQueueSubmit(context_manager_sptr->computeQueue, 1, &submit, VK_NULL_HANDLE);
	vkQueueWaitIdle(context_manager_sptr->computeQueue);
	vkFreeCommandBuffers(context_manager_sptr->device, this->computeCommandPool, 1, &command_buffer);
}

VkCommandBuffer CommandManager::beginGraphicsCommands()
{
	/* Creating a temporary command buffer */
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.commandPool = this->graphicsCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;
	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(context_manager_sptr->device, &allocInfo, &command_buffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;

	vkBeginCommandBuffer(command_buffer, &beginInfo);
	return command_buffer;
}

void CommandManager::endGraphicsCommands(VkCommandBuffer command_buffer)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit{};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;
	submit.waitSemaphoreCount = 0;
	submit.pWaitSemaphores = nullptr;
	submit.pWaitDstStageMask = nullptr;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &command_buffer;
	submit.signalSemaphoreCount = 0;
	submit.pSignalSemaphores = nullptr;

	vkQueueSubmit(context_manager_sptr->graphicsQueue, 1, &submit, VK_NULL_HANDLE);
	vkQueueWaitIdle(context_manager_sptr->graphicsQueue);
	vkFreeCommandBuffers(context_manager_sptr->device, this->graphicsCommandPool, 1, &command_buffer);
}