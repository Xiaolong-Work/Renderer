#include <pipeline_manager.h>

PipelineManager::PipelineManager(const ContextManagerSPtr& context_manager_sptr)
{
	this->context_manager_sptr = context_manager_sptr;
}

void PipelineManager::init()
{
	createPipeline();
}

void PipelineManager::clear()
{
	vkDestroyPipeline(context_manager_sptr->device, this->pipeline, nullptr);
	vkDestroyPipelineLayout(context_manager_sptr->device, this->layout, nullptr);
}

void PipelineManager::setRequiredValue(const std::vector<VkPipelineShaderStageCreateInfo>& shader_stages,
									   const VkViewport& viewport,
									   const VkRect2D& scissor,
									   const VkPipelineLayoutCreateInfo& pipelineLayoutInfo,
									   const VkRenderPass& pass)
{
	this->shader_stages = shader_stages;
	this->viewport = viewport;
	this->scissor = scissor;
	this->pipelineLayoutInfo = pipelineLayoutInfo;
	this->pass = pass;

	setDefaultFixedState();
}

void PipelineManager::createPipeline()
{
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	auto bindingDescription = VertexBufferManager::getBindingDescription();
	auto attributeDescriptions = VertexBufferManager::getAttributeDescriptions();

	if (this->enable_vertex_inpute)
	{
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	}
	else
	{
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;
	}

	VkPipelineDynamicStateCreateInfo dynamic_state{};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.pNext = nullptr;
	dynamic_state.flags = 0;
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
	dynamic_state.pDynamicStates = dynamic_states.data();

	VkPipelineViewportStateCreateInfo viewport_state{};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.pNext = nullptr;
	viewport_state.flags = 0;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &this->viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &this->scissor;

	if (vkCreatePipelineLayout(context_manager_sptr->device, &this->pipelineLayoutInfo, nullptr, &this->layout) !=
		VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = this->next_pointer;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = static_cast<uint32_t>(this->shader_stages.size());
	pipelineInfo.pStages = this->shader_stages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &this->input_assembly;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.pViewportState = &viewport_state;
	pipelineInfo.pRasterizationState = &this->rasterizer;
	pipelineInfo.pMultisampleState = &this->multisample;
	pipelineInfo.pDepthStencilState = &this->depth_stencil;
	if (this->enable_color_attachment)
	{
		pipelineInfo.pColorBlendState = &this->color_blending;
	}
	else
	{
		pipelineInfo.pColorBlendState = nullptr;
	}
	pipelineInfo.pDynamicState = &dynamic_state;
	pipelineInfo.layout = this->layout;
	pipelineInfo.renderPass = pass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(
			context_manager_sptr->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics pipeline!");
	}
}

void PipelineManager::setDefaultFixedState()
{
	this->input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	this->input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	this->input_assembly.primitiveRestartEnable = VK_FALSE;

	this->rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	this->rasterizer.pNext = nullptr;
	this->rasterizer.flags = 0;
	this->rasterizer.depthClampEnable = VK_FALSE;
	this->rasterizer.rasterizerDiscardEnable = VK_FALSE;
	this->rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	this->rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	this->rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	this->rasterizer.depthBiasEnable = VK_FALSE;
	this->rasterizer.depthBiasConstantFactor = 0.0f;
	this->rasterizer.depthBiasClamp = 0.0f;
	this->rasterizer.depthBiasSlopeFactor = 0.0f;
	this->rasterizer.lineWidth = 1.0f;

	this->multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	this->multisample.pNext = nullptr;
	this->multisample.flags = 0;
	this->multisample.sampleShadingEnable = VK_FALSE;
	this->multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	this->multisample.minSampleShading = 1.0f;
	this->multisample.pSampleMask = nullptr;
	this->multisample.alphaToCoverageEnable = VK_FALSE;
	this->multisample.alphaToOneEnable = VK_FALSE;

	this->depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	this->depth_stencil.pNext = nullptr;
	this->depth_stencil.flags = 0;
	this->depth_stencil.depthTestEnable = VK_TRUE;
	this->depth_stencil.depthWriteEnable = VK_TRUE;
	this->depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
	this->depth_stencil.depthBoundsTestEnable = VK_FALSE;
	this->depth_stencil.stencilTestEnable = VK_FALSE;
	this->depth_stencil.front = {};
	this->depth_stencil.back = {};
	this->depth_stencil.minDepthBounds = 0.0f;
	this->depth_stencil.maxDepthBounds = 1.0f;

	this->color_blend_attachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	this->color_blend_attachment.blendEnable = VK_FALSE;
	this->color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	this->color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	this->color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	this->color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	this->color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	this->color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

	this->color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	this->color_blending.pNext = nullptr;
	this->color_blending.flags = 0;
	this->color_blending.logicOpEnable = VK_FALSE;
	this->color_blending.logicOp = VK_LOGIC_OP_COPY;
	this->color_blending.attachmentCount = 1;
	this->color_blending.pAttachments = &color_blend_attachment;
	this->color_blending.blendConstants[0] = 0.0f;
	this->color_blending.blendConstants[1] = 0.0f;
	this->color_blending.blendConstants[2] = 0.0f;
	this->color_blending.blendConstants[3] = 0.0f;
}