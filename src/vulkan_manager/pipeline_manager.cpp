#include <pipeline_manager.h>

PipelineManager::PipelineManager(const ContentManagerSPtr& pContentManager)
{
	this->pContentManager = pContentManager;
}

void PipelineManager::init()
{
	createPipeline();
}

void PipelineManager::clear()
{
	vkDestroyPipeline(pContentManager->device, this->pipeline, nullptr);
	vkDestroyPipelineLayout(pContentManager->device, this->layout, nullptr);
}

void PipelineManager::setRequiredValue(const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
									   const VkViewport& viewport,
									   const VkRect2D& scissor,
									   const VkPipelineLayoutCreateInfo& pipelineLayoutInfo,
									   const VkRenderPass& renderPass)
{
	this->shaderStages = shaderStages;
	this->viewport = viewport;
	this->scissor = scissor;
	this->pipelineLayoutInfo = pipelineLayoutInfo;
	this->renderPass = renderPass;

	setDefaultFixedState();
}

void PipelineManager::createPipeline()
{
	auto bindingDescription = Vertex1::getBindingDescription();
	auto attributeDescriptions = Vertex1::getAttributeDescriptions();
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pNext = nullptr;
	dynamicState.flags = 0;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;
	viewportState.flags = 0;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &this->viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &this->scissor;

	if (vkCreatePipelineLayout(pContentManager->device, &this->pipelineLayoutInfo, nullptr, &this->layout) !=
		VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = static_cast<uint32_t>(this->shaderStages.size());
	pipelineInfo.pStages = this->shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &this->inputAssembly;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &this->rasterizer;
	pipelineInfo.pMultisampleState = &this->multisampling;
	pipelineInfo.pDepthStencilState = &this->depthStencil;
	pipelineInfo.pColorBlendState = &this->colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = this->layout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(
			pContentManager->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics pipeline!");
	}
}

void PipelineManager::setDefaultFixedState()
{
	this->inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	this->inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	this->inputAssembly.primitiveRestartEnable = VK_FALSE;

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

	this->multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	this->multisampling.pNext = nullptr;
	this->multisampling.flags = 0;
	this->multisampling.sampleShadingEnable = VK_FALSE;
	this->multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	this->multisampling.minSampleShading = 1.0f;
	this->multisampling.pSampleMask = nullptr;
	this->multisampling.alphaToCoverageEnable = VK_FALSE;
	this->multisampling.alphaToOneEnable = VK_FALSE;

	this->depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	this->depthStencil.pNext = nullptr;
	this->depthStencil.flags = 0;
	this->depthStencil.depthTestEnable = VK_TRUE;
	this->depthStencil.depthWriteEnable = VK_TRUE;
	this->depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	this->depthStencil.depthBoundsTestEnable = VK_FALSE;
	this->depthStencil.stencilTestEnable = VK_FALSE;
	this->depthStencil.front = {};
	this->depthStencil.back = {};
	this->depthStencil.minDepthBounds = 0.0f;
	this->depthStencil.maxDepthBounds = 1.0f;

	this->colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	this->colorBlendAttachment.blendEnable = VK_FALSE;
	this->colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	this->colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	this->colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	this->colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	this->colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	this->colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	this->colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	this->colorBlending.pNext = nullptr;
	this->colorBlending.flags = 0;
	this->colorBlending.logicOpEnable = VK_FALSE;
	this->colorBlending.logicOp = VK_LOGIC_OP_COPY;
	this->colorBlending.attachmentCount = 1;
	this->colorBlending.pAttachments = &colorBlendAttachment;
	this->colorBlending.blendConstants[0] = 0.0f;
	this->colorBlending.blendConstants[1] = 0.0f;
	this->colorBlending.blendConstants[2] = 0.0f;
	this->colorBlending.blendConstants[3] = 0.0f;
}