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
	PipelineManager(const ContextManagerSPtr& pContentManager);

	void init();
	void clear();

	void setRequiredValue(const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
						  const VkViewport& viewport,
						  const VkRect2D& scissor,
						  const VkPipelineLayoutCreateInfo& pipelineLayoutInfo,
						  const VkRenderPass& renderPass);

	void createPipeline();

	/* Optional Values */
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	VkPipelineMultisampleStateCreateInfo multisampling{};
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	std::vector<VkDynamicState> dynamicStates;

	VkPipelineLayout layout;
	VkPipeline pipeline;

	bool enableVertexInpute{true};

protected:
	void setDefaultFixedState();

private:
	/* Required value */
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	VkViewport viewport{};
	VkRect2D scissor{};
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	VkRenderPass renderPass;

	ContextManagerSPtr pContentManager;
};
