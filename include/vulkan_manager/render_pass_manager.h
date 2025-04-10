#pragma once

#include <context_manager.h>
#include <image_manager.h>
#include <swap_chain_manager.h>

#include <array>

class RenderPassManager
{
public:
	RenderPassManager() = default;
	RenderPassManager(const ContextManagerSPtr& pContentManager,
					  const SwapChainManagerSPtr& pSwapChainManager,
					  const CommandManagerSPtr& pCommandManager);

	void init();
	void clear();

	void recreate();

	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffers;
	DepthImageManager depthImageManager;

	SwapChainManagerSPtr pSwapChainManager;

protected:
	void createRenderPass();

	void createFramebuffers();

private:
	ContextManagerSPtr pContentManager;
	CommandManagerSPtr pCommandManager;
};
