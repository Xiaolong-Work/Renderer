#pragma once

#include <context_manager.h>
#include <image_manager.h>
#include <swap_chain_manager.h>

#include <array>

enum class RenderPassType
{
	Render = 0,
	Shadow
};

class RenderPassManager
{
public:
	RenderPassManager() = default;
	RenderPassManager(const ContextManagerSPtr& context_manager_sptr,
					  const SwapChainManagerSPtr& swap_chain_manager_sprt,
					  const CommandManagerSPtr& command_manager_sptr,
					  const RenderPassType type = RenderPassType::Render);

	RenderPassManager(const ContextManagerSPtr& context_manager_sptr,
					  const CommandManagerSPtr& command_manager_sptr,
					  const RenderPassType type = RenderPassType::Shadow);

	void init();
	void clear();

	void recreate();

	VkRenderPass pass;
	std::vector<VkFramebuffer> buffers;
	DepthImageManager depthImageManager;

	SwapChainManagerSPtr swap_chain_manager_sprt;

protected:
	void createRenderPass();

	void createRenderPassForShadowCompute();

	void createFrameBuffers();
	
	void createFrameBuffersForShadowCompute();

private:
	ContextManagerSPtr context_manager_sptr;
	CommandManagerSPtr command_manager_sptr;

	RenderPassType type;
};
