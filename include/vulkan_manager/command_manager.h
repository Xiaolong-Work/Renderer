#pragma once

#include <memory>
#include <stdexcept>
#include <vector>

#include <content_manager.h>

class CommandManager
{
public:
	CommandManager() = default;
	CommandManager(const ContentManagerSPtr& pContentManager);

	void init();
	void clear();

	VkCommandBuffer beginTransferCommands();
	void endTransferCommands(VkCommandBuffer commandBuffer);

	VkCommandBuffer beginComputeCommands();
	void endComputeCommands(VkCommandBuffer commandBuffer);

	std::vector<VkCommandBuffer> graphicsCommandBuffers;

protected:
	void createCommandPool();

	void createCommandBuffers();

private:
	VkCommandPool graphicsCommandPool = VK_NULL_HANDLE;
	VkCommandPool transferCommandPool = VK_NULL_HANDLE;
	VkCommandPool computeCommandPool = VK_NULL_HANDLE;

	ContentManagerSPtr pContentManager;
};

typedef std::shared_ptr<CommandManager> CommandManagerSPtr;
