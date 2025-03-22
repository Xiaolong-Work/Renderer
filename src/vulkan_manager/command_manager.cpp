#include <command_manager.h>

CommandManager::CommandManager(const ContentManagerSPtr& pContentManager) { this->pContentManager = pContentManager; }

void CommandManager::init()
{
    createCommandPool();
    createCommandBuffers();
}

void CommandManager::clear()
{
    vkDestroyCommandPool(pContentManager->device, this->graphicsCommandPool, nullptr);
    vkDestroyCommandPool(pContentManager->device, this->transferCommandPool, nullptr);
	vkDestroyCommandPool(pContentManager->device, this->computeCommandPool, nullptr);
}

void CommandManager::createCommandPool()
{
    VkCommandPoolCreateInfo graphicsPoolInfo{};
    graphicsPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    graphicsPoolInfo.pNext = nullptr;
    graphicsPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    graphicsPoolInfo.queueFamilyIndex = pContentManager->graphicsFamily;

    VkCommandPoolCreateInfo transferPoolInfo{};
    transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    transferPoolInfo.pNext = nullptr;
    transferPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    transferPoolInfo.queueFamilyIndex = pContentManager->transferFamily;

    VkCommandPoolCreateInfo computePoolInfo{};
    computePoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    computePoolInfo.pNext = nullptr;
    computePoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    computePoolInfo.queueFamilyIndex = pContentManager->computeFamily;

    if (vkCreateCommandPool(pContentManager->device, &graphicsPoolInfo, nullptr, &this->graphicsCommandPool) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics command pool!");
    }

    if (vkCreateCommandPool(pContentManager->device, &transferPoolInfo, nullptr, &this->transferCommandPool) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create transfer command pool!");
    }

    if (vkCreateCommandPool(pContentManager->device, &computePoolInfo, nullptr, &this->computeCommandPool) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create compute command pool!");
    }
}

void CommandManager::createCommandBuffers()
{
    graphicsCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBuffer temp;
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.commandPool = graphicsCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)graphicsCommandBuffers.size();

    if (vkAllocateCommandBuffers(pContentManager->device, &allocInfo, graphicsCommandBuffers.data()) != VK_SUCCESS)
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
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(pContentManager->device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void CommandManager::endTransferCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;

    vkQueueSubmit(pContentManager->transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(pContentManager->transferQueue);
    vkFreeCommandBuffers(pContentManager->device, transferCommandPool, 1, &commandBuffer);
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
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(pContentManager->device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void CommandManager::endComputeCommands(VkCommandBuffer commandBuffer)
{
    auto ret0 = vkEndCommandBuffer(commandBuffer);

	VkFence fence;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	if (vkCreateFence(pContentManager->device, &fenceInfo, nullptr, &fence) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create fence!");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;

    vkQueueSubmit(pContentManager->computeQueue, 1, &submitInfo, fence);

    vkWaitForFences(pContentManager->device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkFreeCommandBuffers(pContentManager->device, computeCommandPool, 1, &commandBuffer);
}