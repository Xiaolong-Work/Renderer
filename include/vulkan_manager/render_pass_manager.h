#pragma once

#include <context_manager.h>
#include <image_manager.h>
#include <swap_chain_manager.h>

#include <array>
#include <iostream>
#include <optional>

enum class RenderPassType
{
	Render = 0,
	Shadow,
	GBuffer,
	Custom
};

struct AttachmentReference
{
	std::vector<VkAttachmentReference> color;
	std::vector<VkAttachmentReference> input;
	std::vector<uint32_t> preserve;

	std::optional<VkAttachmentReference> depth;
	std::optional<VkAttachmentReference> resolve;
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
	DepthImageManager depth_image_manager;

	SwapChainManagerSPtr swap_chain_manager_sprt;

	void addColorAttachment(const VkFormat format,
							const VkImageLayout final_layout,
							const VkImageLayout using_layout,
							const VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
							const VkAttachmentLoadOp load = VK_ATTACHMENT_LOAD_OP_CLEAR,
							const VkAttachmentStoreOp store = VK_ATTACHMENT_STORE_OP_STORE);

	void addColorAttachment(const VkFormat format,
							const VkSampleCountFlagBits sample,
							const VkAttachmentLoadOp load,
							const VkAttachmentStoreOp store,
							const VkAttachmentLoadOp stencil_load,
							const VkAttachmentStoreOp stencil_store,
							const VkImageLayout initial_layout,
							const VkImageLayout final_layout,
							const VkImageLayout using_layout);

	void addSubpass(const AttachmentReference& reference,
					const VkPipelineBindPoint bind = VK_PIPELINE_BIND_POINT_GRAPHICS,
					const VkSubpassDescriptionFlags flags = 0)
	{
		VkSubpassDescription subpass{};
		subpass.flags = flags;
		subpass.pipelineBindPoint = bind;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = nullptr;
		subpass.colorAttachmentCount = 0;
		subpass.pColorAttachments = nullptr;
		subpass.pResolveAttachments = nullptr;
		subpass.pDepthStencilAttachment = nullptr;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = nullptr;

		this->attachment_references.push_back(reference);
		this->subpasses.push_back(subpass);
	}

	void addDependencyt(const uint32_t srouce, const uint32_t destination)
	{
		VkSubpassDependency dependency{};
		dependency.srcSubpass = srouce;
		dependency.dstSubpass = destination;

		dependency.srcStageMask =
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask =
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency.dependencyFlags = 0;

		addDependency(dependency);
	}

	void addDependency(const VkSubpassDependency& dependency)
	{
		this->dependencies.push_back(dependency);
	}

	void setDefaultDepthAttachment();

	std::vector<std::vector<VkImageView>> views;


	void createRenderPass();

	void configureForForwardRender();

	void configureForShadowCompute();

	void configureForDeferredGeometryBuffer();

	void createFrameBuffers();

	void createFrameBuffersForShadowCompute();

	void setDefaultSubpass();

	void setDefaultDependency();


	ContextManagerSPtr context_manager_sptr;
	CommandManagerSPtr command_manager_sptr;

	RenderPassType type;

	std::vector<VkAttachmentDescription> attachment_descriptions{};
	std::vector<AttachmentReference> attachment_references{};
	std::vector<VkSubpassDescription> subpasses{};
	std::vector<VkSubpassDependency> dependencies{};

	VkAttachmentDescription depth_attachment_description{};
	VkAttachmentReference depth_attachment_reference{};

	

	void* next{nullptr};
};
