#pragma once

#include <memory>
#include <stdexcept>
#include <vector>

#include <context_manager.h>

class CommandManager
{
public:
	CommandManager() = default;
	CommandManager(const ContextManagerSPtr& context_manager_sptr);

	void init();
	void clear();

	VkCommandBuffer beginTransferCommands();
	void endTransferCommands(VkCommandBuffer commandBuffer);

	VkCommandBuffer beginComputeCommands();
	void endComputeCommands(VkCommandBuffer commandBuffer);

	VkCommandBuffer beginGraphicsCommands();
	void endGraphicsCommands(VkCommandBuffer commandBuffer);

	std::vector<VkCommandBuffer> graphicsCommandBuffers;

protected:
	void createCommandPool();

	void createCommandBuffers();

private:
	VkCommandPool graphicsCommandPool = VK_NULL_HANDLE;
	VkCommandPool transferCommandPool = VK_NULL_HANDLE;
	VkCommandPool computeCommandPool = VK_NULL_HANDLE;

	ContextManagerSPtr context_manager_sptr;
};

typedef std::shared_ptr<CommandManager> CommandManagerSPtr;
