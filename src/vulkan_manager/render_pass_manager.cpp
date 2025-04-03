#include <render_pass_manager.h>

RenderPassManager::RenderPassManager(const ContextManagerSPtr& pContentManager,
									 const SwapChainManagerSPtr& pSwapChainManager,
									 const CommandManagerSPtr& pCommandManager)
{
	this->pContentManager = pContentManager;
	this->pSwapChainManager = pSwapChainManager;
	this->pCommandManager = pCommandManager;
}

void RenderPassManager::init()
{
	this->depthImageManager = DepthImageManager(this->pContentManager, this->pCommandManager);
	this->depthImageManager.setExtent(pSwapChainManager->extent);
	this->depthImageManager.init();

	createRenderPass();

	createFramebuffers();
}

void RenderPassManager::clear()
{
	for (auto framebuffer : framebuffers)
	{
		vkDestroyFramebuffer(pContentManager->device, framebuffer, nullptr);
	}

	vkDestroyRenderPass(pContentManager->device, renderPass, nullptr);

	this->depthImageManager.clear();
}

void RenderPassManager::recreate()
{
	for (auto framebuffer : framebuffers)
	{
		vkDestroyFramebuffer(pContentManager->device, framebuffer, nullptr);
	}
	this->depthImageManager.clear();

	this->depthImageManager.setExtent(pSwapChainManager->extent);
	this->depthImageManager.init();
	createFramebuffers();
}

void RenderPassManager::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.flags = 0;
	colorAttachment.format = pSwapChainManager->format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.flags = 0;
	depthAttachment.format = depthImageManager.format;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags = 0;

	std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(pContentManager->device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create render pass!");
	}
}

void RenderPassManager::createFramebuffers()
{
	this->framebuffers.resize(pSwapChainManager->imageViews.size());

	for (size_t i = 0; i < pSwapChainManager->imageViews.size(); i++)
	{
		std::array<VkImageView, 2> attachments = {pSwapChainManager->imageViews[i], this->depthImageManager.imageView};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.pNext = nullptr;
		framebufferInfo.flags = 0;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = pSwapChainManager->extent.width;
		framebufferInfo.height = pSwapChainManager->extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(pContentManager->device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create framebuffer!");
		}
	}
}
