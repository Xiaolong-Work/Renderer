#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>

#include <buffer_manager.h>
#include <context_manager.h>

class PipelineManager
{
public:
	PipelineManager() = default;
	PipelineManager(const ContextManagerSPtr& context_manager_sptr);

	void init();
	void clear();

	void setRequiredValue(const std::vector<VkPipelineShaderStageCreateInfo>& shader_stages,
						  const VkViewport& viewport,
						  const VkRect2D& scissor,
						  const VkPipelineLayoutCreateInfo& pipelineLayoutInfo,
						  const VkRenderPass& pass);

	void createPipeline();

	/* Optional Values */
	VkPipelineInputAssemblyStateCreateInfo input_assembly{};
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	VkPipelineMultisampleStateCreateInfo multisample{};
	VkPipelineDepthStencilStateCreateInfo depth_stencil{};
	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	VkPipelineColorBlendStateCreateInfo color_blending{};
	std::vector<VkDynamicState> dynamic_states;

	VkPipelineLayout layout;
	VkPipeline pipeline;

	bool enable_vertex_inpute{true};
	bool enable_color_attachment{true};

	void* next_pointer = nullptr;

protected:
	void setDefaultFixedState();

private:
	/* Required value */
	std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
	VkViewport viewport{};
	VkRect2D scissor{};
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	VkRenderPass pass;

	ContextManagerSPtr context_manager_sptr;
};
