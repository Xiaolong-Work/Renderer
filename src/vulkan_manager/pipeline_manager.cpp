#include <pipeline_manager.h>

PipelineManager::PipelineManager(const ContextManagerSPtr& context_manager_sptr, const PipelineType type)
{
	this->context_manager_sptr = context_manager_sptr;
	this->type = type;
}

void PipelineManager::init()
{
	if (this->type == PipelineType::Rasterize)
	{
		createPipeline();
	}
	else if (this->type == PipelineType::PathTracing)
	{
		createRayTracingPipeline();
	}
}

void PipelineManager::clear()
{
	for (auto& shader_manager : this->shader_managers)
	{
		shader_manager.clear();
	}

	vkDestroyPipeline(context_manager_sptr->device, this->pipeline, nullptr);
	vkDestroyPipelineLayout(context_manager_sptr->device, this->layout, nullptr);
}

void PipelineManager::addShaderStage(const std::string& name, const VkShaderStageFlagBits stage)
{
	ShaderManager shader(this->context_manager_sptr);
	shader.setShaderName(name);
	shader.init();
	this->shader_managers.push_back(shader);

	VkPipelineShaderStageCreateInfo shader_stage{};
	shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage.pNext = nullptr;
	shader_stage.flags = 0;
	shader_stage.stage = stage;
	shader_stage.module = this->shader_managers.back().module;
	shader_stage.pName = "main";
	shader_stage.pSpecializationInfo = nullptr;
	this->shader_stages.push_back(shader_stage);
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
	this->rasterizer.cullMode = VK_CULL_MODE_NONE;
	this->rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	this->rasterizer.depthBiasEnable = VK_FALSE;
	this->rasterizer.depthBiasConstantFactor = 0.0f;
	this->rasterizer.depthBiasClamp = 0.0f;
	this->rasterizer.depthBiasSlopeFactor = 0.0f;
	this->rasterizer.lineWidth = 1.0f;

	this->multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	this->multisample.pNext = nullptr;
	this->multisample.flags = 0;
	this->multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	this->multisample.sampleShadingEnable = VK_FALSE;
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

	this->color_blend_attachments.resize(1);
	this->color_blend_attachments[0].blendEnable = VK_FALSE;
	this->color_blend_attachments[0].colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	this->color_blend_attachments[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	this->color_blend_attachments[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	this->color_blend_attachments[0].colorBlendOp = VK_BLEND_OP_ADD;
	this->color_blend_attachments[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	this->color_blend_attachments[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	this->color_blend_attachments[0].alphaBlendOp = VK_BLEND_OP_ADD;

	this->color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	this->color_blending.pNext = nullptr;
	this->color_blending.flags = 0;
	this->color_blending.logicOpEnable = VK_FALSE;
	this->color_blending.logicOp = VK_LOGIC_OP_COPY;
	this->color_blending.attachmentCount = static_cast<uint32_t>(this->color_blend_attachments.size());
	this->color_blending.pAttachments = this->color_blend_attachments.data();
	this->color_blending.blendConstants[0] = 0.0f;
	this->color_blending.blendConstants[1] = 0.0f;
	this->color_blending.blendConstants[2] = 0.0f;
	this->color_blending.blendConstants[3] = 0.0f;
}

void PipelineManager::setExtent(const VkExtent2D extent)
{
	this->viewport.x = 0.0f;
	this->viewport.y = 0.0f;
	this->viewport.width = static_cast<float>(extent.width);
	this->viewport.height = static_cast<float>(extent.height);
	this->viewport.minDepth = 0.0f;
	this->viewport.maxDepth = 1.0f;

	this->scissor.offset = {0, 0};
	this->scissor.extent = extent;

	this->viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	this->viewport_state.pNext = nullptr;
	this->viewport_state.flags = 0;
	this->viewport_state.viewportCount = 1;
	this->viewport_state.pViewports = &this->viewport;
	this->viewport_state.scissorCount = 1;
	this->viewport_state.pScissors = &this->scissor;
}

void PipelineManager::setRenderPass(const VkRenderPass render_pass, const uint32_t subpass)
{
	this->render_pass = render_pass;
	this->subpass = subpass;
}

void PipelineManager::setVertexInput(const uint32_t mask)
{
	if ((mask & 0b1000) >> 3 == 1)
	{
		VkVertexInputAttributeDescription attribute{};
		attribute.binding = 0;
		attribute.location = 0;
		attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute.offset = offsetof(Vertex, position);
		this->attribute_descriptions.push_back(attribute);
	}
	if ((mask & 0b100) >> 2 == 1)
	{
		VkVertexInputAttributeDescription attribute{};
		attribute.binding = 0;
		attribute.location = 1;
		attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute.offset = offsetof(Vertex, normal);
		this->attribute_descriptions.push_back(attribute);
	}
	if ((mask & 0b10) >> 1 == 1)
	{
		VkVertexInputAttributeDescription attribute{};
		attribute.binding = 0;
		attribute.location = 2;
		attribute.format = VK_FORMAT_R32G32_SFLOAT;
		attribute.offset = offsetof(Vertex, texture);
		this->attribute_descriptions.push_back(attribute);
	}
	if ((mask & 0b1) >> 0 == 1)
	{
		VkVertexInputAttributeDescription attribute{};
		attribute.binding = 0;
		attribute.location = 3;
		attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attribute.offset = offsetof(Vertex, color);
		this->attribute_descriptions.push_back(attribute);
	}
}

void PipelineManager::setLayout(const std::vector<VkDescriptorSetLayout>& descriptor_layouts,
								const std::vector<VkPushConstantRange>& push_constants)
{
	this->descriptor_layouts = descriptor_layouts;
	this->push_constants = push_constants;

	this->pipeline_layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	this->pipeline_layout.pNext = nullptr;
	this->pipeline_layout.flags = 0;
	this->pipeline_layout.setLayoutCount = static_cast<uint32_t>(this->descriptor_layouts.size());
	this->pipeline_layout.pSetLayouts = this->descriptor_layouts.data();
	this->pipeline_layout.pushConstantRangeCount = static_cast<uint32_t>(this->push_constants.size());
	this->pipeline_layout.pPushConstantRanges = this->push_constants.data();

	if (vkCreatePipelineLayout(context_manager_sptr->device, &this->pipeline_layout, nullptr, &this->layout) !=
		VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout!");
	}
}

void PipelineManager::setShaderGroups(const std::vector<VkRayTracingShaderGroupCreateInfoKHR>& groups)
{
	this->shader_groups = groups;
}

void PipelineManager::createPipeline()
{
	VkVertexInputBindingDescription binding_description{};
	binding_description.binding = 0;
	binding_description.stride = sizeof(Vertex);
	binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	VkPipelineVertexInputStateCreateInfo vertex_input{};
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input.pNext = nullptr;
	vertex_input.flags = 0;
	vertex_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(this->attribute_descriptions.size());
	vertex_input.pVertexAttributeDescriptions = this->attribute_descriptions.data();
	vertex_input.vertexBindingDescriptionCount = 0;
	vertex_input.pVertexBindingDescriptions = nullptr;
	if (!this->attribute_descriptions.empty())
	{
		vertex_input.vertexBindingDescriptionCount = 1;
		vertex_input.pVertexBindingDescriptions = &binding_description;
	}

	VkPipelineDynamicStateCreateInfo dynamic_state{};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.pNext = nullptr;
	dynamic_state.flags = 0;
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(this->dynamic_states.size());
	dynamic_state.pDynamicStates = this->dynamic_states.data();

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = this->next;
	pipelineInfo.flags = 0;
	pipelineInfo.stageCount = static_cast<uint32_t>(this->shader_stages.size());
	pipelineInfo.pStages = this->shader_stages.data();
	pipelineInfo.pVertexInputState = &vertex_input;

	pipelineInfo.pInputAssemblyState = &this->input_assembly;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.pViewportState = &this->viewport_state;
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
	pipelineInfo.renderPass = this->render_pass;
	pipelineInfo.subpass = subpass;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(
			context_manager_sptr->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics pipeline!");
	}
}

void PipelineManager::createRayTracingPipeline()
{
	VkPipelineDynamicStateCreateInfo dynamic_state{};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.pNext = nullptr;
	dynamic_state.flags = 0;
	dynamic_state.dynamicStateCount = static_cast<uint32_t>(this->dynamic_states.size());
	dynamic_state.pDynamicStates = this->dynamic_states.data();

	VkRayTracingPipelineCreateInfoKHR ray_tracing_pipeline_create{};
	ray_tracing_pipeline_create.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	ray_tracing_pipeline_create.pNext = nullptr;
	ray_tracing_pipeline_create.flags = 0;
	ray_tracing_pipeline_create.stageCount = static_cast<uint32_t>(this->shader_stages.size());
	ray_tracing_pipeline_create.pStages = this->shader_stages.data();
	ray_tracing_pipeline_create.groupCount = static_cast<uint32_t>(this->shader_groups.size());
	ray_tracing_pipeline_create.pGroups = this->shader_groups.data();
	ray_tracing_pipeline_create.maxPipelineRayRecursionDepth = 10;
	ray_tracing_pipeline_create.pLibraryInfo = nullptr;
	ray_tracing_pipeline_create.pLibraryInterface = nullptr;
	ray_tracing_pipeline_create.pDynamicState = &dynamic_state;
	ray_tracing_pipeline_create.layout = this->layout;
	ray_tracing_pipeline_create.basePipelineHandle = VK_NULL_HANDLE;
	ray_tracing_pipeline_create.basePipelineIndex = -1;

	auto vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(
		context_manager_sptr->device, "vkCreateRayTracingPipelinesKHR");

	if (vkCreateRayTracingPipelinesKHR(context_manager_sptr->device,
									   VK_NULL_HANDLE,
									   VK_NULL_HANDLE,
									   1,
									   &ray_tracing_pipeline_create,
									   nullptr,
									   &this->pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create ray tracing pipeline!");
	}
}
