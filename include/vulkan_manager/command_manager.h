#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include <content_manager.h>

const int MAX_FRAMES_IN_FLIGHT = 2;

class CommandManager
{
public:
    CommandManager() = default;
    CommandManager(const ContentManagerSPtr& pContentManager);

    void init();
    void clear();

    void createCommandPool();
    void createCommandBuffers();
    std::vector<VkCommandBuffer> graphicsCommandBuffers;

    VkCommandBuffer beginTransferCommands();
    void endTransferCommands(VkCommandBuffer commandBuffer);

    VkCommandBuffer beginComputeCommands();

    void endComputeCommands(VkCommandBuffer commandBuffer);

private:
    VkCommandPool graphicsCommandPool = VK_NULL_HANDLE;
    VkCommandPool transferCommandPool = VK_NULL_HANDLE;
    VkCommandPool computeCommandPool = VK_NULL_HANDLE;

    ContentManagerSPtr pContentManager;
};

typedef std::shared_ptr<CommandManager> CommandManagerSPtr;
