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
		this->depth_image_manager = DepthImageManager(this->context_manager_sptr, this->command_manager_sptr);
		this->depth_image_manager.setExtent(swap_chain_manager_sprt->extent);
		this->depth_image_manager.init();

		configureForForwardRender();

		createRenderPass();
		createFrameBuffers();
	}
	else if (this->type == RenderPassType::Shadow)
	{
		configureForShadowCompute();

		createRenderPass();
	}
	else if (this->type == RenderPassType::GBuffer)
	{
		configureForDeferredGeometryBuffer();

		createRenderPass();
	}
	else if (this->type == RenderPassType::Custom)
	{
		this->depth_image_manager = DepthImageManager(this->context_manager_sptr, this->command_manager_sptr);
		this->depth_image_manager.setExtent(swap_chain_manager_sprt->extent);
		this->depth_image_manager.init();

		this->setDefaultDepthAttachment();
		this->attachment_references[0].depth = this->depth_attachment_reference;

		createRenderPass();

		createFrameBuffers();
	}
}

void RenderPassManager::clear()
{
	for (auto framebuffer : buffers)
	{
		vkDestroyFramebuffer(context_manager_sptr->device, framebuffer, nullptr);
	}

	vkDestroyRenderPass(context_manager_sptr->device, pass, nullptr);

	if (this->type == RenderPassType::Render)
	{
		this->depth_image_manager.clear();
	}
}

void RenderPassManager::recreate()
{
	for (auto framebuffer : buffers)
	{
		vkDestroyFramebuffer(context_manager_sptr->device, framebuffer, nullptr);
	}
	this->depth_image_manager.clear();

	this->depth_image_manager.setExtent(swap_chain_manager_sprt->extent);
	this->depth_image_manager.init();

	this->views.clear();
	this->views.resize(this->swap_chain_manager_sprt->views.size());
	for (size_t i = 0; i < this->swap_chain_manager_sprt->views.size(); i++)
	{
		this->views[i] = {this->swap_chain_manager_sprt->views[i]};
	}

	createFrameBuffers();
}

void RenderPassManager::createRenderPass()
{
	std::vector<VkAttachmentDescription> descriptions = this->attachment_descriptions;
	descriptions.push_back(depth_attachment_description);

	for (size_t i = 0; i < this->subpasses.size(); i++)
	{
		auto& reference = this->attachment_references[i];
		auto& subpass = this->subpasses[i];

		subpass.colorAttachmentCount = static_cast<uint32_t>(reference.color.size());
		subpass.pColorAttachments = reference.color.data();

		subpass.inputAttachmentCount = static_cast<uint32_t>(reference.input.size());
		subpass.pInputAttachments = reference.input.data();

		subpass.preserveAttachmentCount = static_cast<uint32_t>(reference.preserve.size());
		subpass.pPreserveAttachments = reference.preserve.data();

		if (reference.depth.has_value())
		{
			subpass.pDepthStencilAttachment = &reference.depth.value();
		}
		else
		{
			subpass.pDepthStencilAttachment = nullptr;
		}

		if (reference.resolve.has_value())
		{
			subpass.pResolveAttachments = &reference.resolve.value();
		}
		else
		{
			subpass.pResolveAttachments = nullptr;
		}
	}

	VkRenderPassCreateInfo render_pass_information{};
	render_pass_information.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_information.pNext = this->next;
	render_pass_information.flags = 0;
	render_pass_information.attachmentCount = static_cast<uint32_t>(descriptions.size());
	render_pass_information.pAttachments = descriptions.data();
	render_pass_information.subpassCount = static_cast<uint32_t>(this->subpasses.size());
	render_pass_information.pSubpasses = this->subpasses.data();
	render_pass_information.dependencyCount = static_cast<uint32_t>(this->dependencies.size());
	render_pass_information.pDependencies = this->dependencies.data();

	if (vkCreateRenderPass(context_manager_sptr->device, &render_pass_information, nullptr, &this->pass) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create render pass!");
	}
}

void RenderPassManager::configureForForwardRender()
{
	this->addColorAttachment(
		swap_chain_manager_sprt->format, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	this->attachment_references.resize(1);
	this->attachment_references[0].color.push_back(VkAttachmentReference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

	this->setDefaultDepthAttachment();
	this->attachment_references[0].depth = this->depth_attachment_reference;

	this->setDefaultSubpass();

	this->setDefaultDependency();

	this->views.resize(this->swap_chain_manager_sprt->views.size());
	for (size_t i = 0; i < this->swap_chain_manager_sprt->views.size(); i++)
	{
		this->views[i] = {this->swap_chain_manager_sprt->views[i]};
	}
}

void RenderPassManager::configureForShadowCompute()
{
	this->addColorAttachment(
		VK_FORMAT_R32_SFLOAT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	this->attachment_references.resize(1);
	this->attachment_references[0].color.push_back(VkAttachmentReference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

	this->setDefaultDepthAttachment();
	this->attachment_references[0].depth = this->depth_attachment_reference;

	this->setDefaultSubpass();

	this->setDefaultDependency();

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

	// this->next = &multiview_create_information;
}

void RenderPassManager::configureForDeferredGeometryBuffer()
{
	for (size_t i = 0; i < 4; i++)
	{
		this->addColorAttachment(VK_FORMAT_R32G32B32A32_SFLOAT,
								 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
								 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
								 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	this->setDefaultDepthAttachment();

	this->setDefaultSubpass();

	this->setDefaultDependency();
}

void RenderPassManager::createFrameBuffers()
{
	this->buffers.resize(swap_chain_manager_sprt->views.size());

	for (size_t i = 0; i < swap_chain_manager_sprt->views.size(); i++)
	{
		this->views[i].push_back(this->depth_image_manager.view);
		VkFramebufferCreateInfo framebuffer_information{};
		framebuffer_information.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_information.pNext = nullptr;
		framebuffer_information.flags = 0;
		framebuffer_information.renderPass = this->pass;
		framebuffer_information.attachmentCount = static_cast<uint32_t>(this->views[i].size());
		framebuffer_information.pAttachments = this->views[i].data();
		framebuffer_information.width = this->swap_chain_manager_sprt->extent.width;
		framebuffer_information.height = this->swap_chain_manager_sprt->extent.height;
		framebuffer_information.layers = 1;

		if (vkCreateFramebuffer(context_manager_sptr->device, &framebuffer_information, nullptr, &buffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create framebuffer!");
		}
	}
}

void RenderPassManager::createFrameBuffersForShadowCompute()
{
}

void RenderPassManager::addColorAttachment(const VkFormat format,
										   const VkImageLayout final_layout,
										   const VkImageLayout using_layout,
										   const VkImageLayout initial_layout,
										   const VkAttachmentLoadOp load,
										   const VkAttachmentStoreOp store)

{
	addColorAttachment(format,
					   VK_SAMPLE_COUNT_1_BIT,
					   load,
					   store,
					   VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					   VK_ATTACHMENT_STORE_OP_DONT_CARE,
					   initial_layout,
					   final_layout,
					   using_layout);
}

void RenderPassManager::addColorAttachment(const VkFormat format,
										   const VkSampleCountFlagBits sample,
										   const VkAttachmentLoadOp load,
										   const VkAttachmentStoreOp store,
										   const VkAttachmentLoadOp stencil_load,
										   const VkAttachmentStoreOp stencil_store,
										   const VkImageLayout initial_layout,
										   const VkImageLayout final_layout,
										   const VkImageLayout using_layout)
{
	VkAttachmentDescription description{};
	description.flags = 0;
	description.format = format;
	description.samples = sample;
	description.loadOp = load;
	description.storeOp = store;
	description.stencilLoadOp = stencil_load;
	description.stencilStoreOp = stencil_store;
	description.initialLayout = initial_layout;
	description.finalLayout = final_layout;
	this->attachment_descriptions.push_back(description);
}

void RenderPassManager::setDefaultDepthAttachment()
{
	this->depth_attachment_description.flags = 0;
	this->depth_attachment_description.format = VK_FORMAT_D32_SFLOAT;
	this->depth_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
	this->depth_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	this->depth_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	this->depth_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	this->depth_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	this->depth_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	this->depth_attachment_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	this->depth_attachment_reference.attachment = this->attachment_descriptions.size();
	this->depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
}

void RenderPassManager::setDefaultSubpass()
{
	VkSubpassDescription subpass{};
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.colorAttachmentCount = static_cast<uint32_t>(this->attachment_references.size());
	subpass.pColorAttachments = this->attachment_references[0].color.data();
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = &this->depth_attachment_reference;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;
	this->subpasses.push_back(subpass);
}

void RenderPassManager::setDefaultDependency()
{
	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstStageMask =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags = 0;
	this->dependencies.push_back(dependency);
}