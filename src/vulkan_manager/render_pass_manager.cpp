#include <render_pass_manager.h>

RenderPassManager::RenderPassManager(const ContextManagerSPtr& context_manager_sptr,
									 const SwapChainManagerSPtr& swap_chain_manager_sprt,
									 const CommandManagerSPtr& command_manager_sptr,
									 const RenderPassType type)
{
	this->context_manager_sptr = context_manager_sptr;
	this->swap_chain_manager_sprt = swap_chain_manager_sprt;
	this->command_manager_sptr = command_manager_sptr;
	this->type = type;
}

RenderPassManager::RenderPassManager(const ContextManagerSPtr& context_manager_sptr,
									 const CommandManagerSPtr& command_manager_sptr,
									 const RenderPassType type)
{
	this->context_manager_sptr = context_manager_sptr;
	this->command_manager_sptr = command_manager_sptr;
	this->type = type;
}

void RenderPassManager::init()
{
	if (this->type == RenderPassType::Render)
	{
		this->depthImageManager = DepthImageManager(this->context_manager_sptr, this->command_manager_sptr);
		this->depthImageManager.setExtent(swap_chain_manager_sprt->extent);
		this->depthImageManager.init();

		createRenderPass();
		createFrameBuffers();
	}
	else if (this->type == RenderPassType::Shadow)
	{
		createRenderPassForShadowCompute();
	}
}

void RenderPassManager::clear()
{
	for (auto framebuffer : buffers)
	{
		vkDestroyFramebuffer(context_manager_sptr->device, framebuffer, nullptr);
	}

	vkDestroyRenderPass(context_manager_sptr->device, pass, nullptr);

	this->depthImageManager.clear();
}

void RenderPassManager::recreate()
{
	for (auto framebuffer : buffers)
	{
		vkDestroyFramebuffer(context_manager_sptr->device, framebuffer, nullptr);
	}
	this->depthImageManager.clear();

	this->depthImageManager.setExtent(swap_chain_manager_sprt->extent);
	this->depthImageManager.init();
	createFrameBuffers();
}

void RenderPassManager::createRenderPass()
{
	VkAttachmentDescription color_attachment{};
	color_attachment.flags = 0;
	color_attachment.format = swap_chain_manager_sprt->format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depth_attachment{};
	depth_attachment.flags = 0;
	depth_attachment.format = depthImageManager.format;
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachment_reference{};
	colorAttachment_reference.attachment = 0;
	colorAttachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachment_reference;
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

	std::array<VkAttachmentDescription, 2> attachments = {color_attachment, depth_attachment};
	VkRenderPassCreateInfo render_pass_information{};
	render_pass_information.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_information.attachmentCount = 2;
	render_pass_information.pAttachments = attachments.data();
	render_pass_information.subpassCount = 1;
	render_pass_information.pSubpasses = &subpass;
	render_pass_information.dependencyCount = 1;
	render_pass_information.pDependencies = &dependency;

	if (vkCreateRenderPass(context_manager_sptr->device, &render_pass_information, nullptr, &this->pass) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create render pass!");
	}
}

void RenderPassManager::createRenderPassForShadowCompute()
{
	VkAttachmentDescription color_attachment{};
	color_attachment.flags = 0;
	color_attachment.format = VK_FORMAT_R32_SFLOAT;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_attachment_reference{};
	color_attachment_reference.attachment = 0;
	color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depth_attachment{};
	depth_attachment.flags = 0;
	depth_attachment.format = VK_FORMAT_D32_SFLOAT;
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_reference{};
	depth_attachment_reference.attachment = 1;
	depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_reference;
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = &depth_attachment_reference;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	/* For the 6 faces of the cubemap */
	uint32_t view_mask = 0b00111111;
	uint32_t correlation_mask = 0b00111111;
	VkRenderPassMultiviewCreateInfo multiview_create_information{};
	multiview_create_information.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
	multiview_create_information.subpassCount = 1;
	multiview_create_information.pViewMasks = &view_mask;
	multiview_create_information.dependencyCount = 0;
	multiview_create_information.pViewOffsets = nullptr;
	multiview_create_information.correlationMaskCount = 1;
	multiview_create_information.pCorrelationMasks = &correlation_mask;

	std::array<VkAttachmentDescription, 2> attachments{color_attachment, depth_attachment};
	VkRenderPassCreateInfo render_pass_information{};
	render_pass_information.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_information.pNext = &multiview_create_information;
	render_pass_information.flags = 0;
	render_pass_information.attachmentCount = 2;
	render_pass_information.pAttachments = attachments.data();
	render_pass_information.subpassCount = 1;
	render_pass_information.pSubpasses = &subpass;
	render_pass_information.dependencyCount = 1;
	render_pass_information.pDependencies = &dependency;

	if (vkCreateRenderPass(context_manager_sptr->device, &render_pass_information, nullptr, &this->pass) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create render pass!");
	}
}

void RenderPassManager::createFrameBuffers()
{
	this->buffers.resize(swap_chain_manager_sprt->imageViews.size());

	for (size_t i = 0; i < swap_chain_manager_sprt->imageViews.size(); i++)
	{
		std::array<VkImageView, 2> attachments = {this->swap_chain_manager_sprt->imageViews[i],
												  this->depthImageManager.imageView};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.pNext = nullptr;
		framebufferInfo.flags = 0;
		framebufferInfo.renderPass = pass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swap_chain_manager_sprt->extent.width;
		framebufferInfo.height = swap_chain_manager_sprt->extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(context_manager_sptr->device, &framebufferInfo, nullptr, &buffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create framebuffer!");
		}
	}
}

void RenderPassManager::createFrameBuffersForShadowCompute()
{
}
