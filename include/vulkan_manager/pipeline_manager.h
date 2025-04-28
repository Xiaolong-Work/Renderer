#pragma once

#include <stdexcept>
#include <vector>

#include <buffer_manager.h>
#include <context_manager.h>
#include <shader_manager.h>

enum class PipelineType
{
	Rasterize,
	PathTracing,
	Compute
};

class PipelineManager
{
public:
	PipelineManager() = default;
	PipelineManager(const ContextManagerSPtr& context_manager_sptr, const PipelineType type = PipelineType::Rasterize);

	void init();
	void clear();

	void addShaderStage(const std::string& name, const VkShaderStageFlagBits stage);

	void setDefaultFixedState();

	void setExtent(const VkExtent2D extent);

	void setRenderPass(const VkRenderPass render_pass, const uint32_t subpass = 0);

	void setVertexInput(const uint32_t mask);

	void setLayout(const std::vector<VkDescriptorSetLayout>& descriptor_layouts,
				   const std::vector<VkPushConstantRange>& push_constants = {});

	void setShaderGroups(const std::vector<VkRayTracingShaderGroupCreateInfoKHR>& groups);

	/* Optional Values */
	void* next{nullptr};
	VkPipelineInputAssemblyStateCreateInfo input_assembly{};
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	VkPipelineMultisampleStateCreateInfo multisample{};
	VkPipelineDepthStencilStateCreateInfo depth_stencil{};
	std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments{};
	VkPipelineColorBlendStateCreateInfo color_blending{};
	std::vector<VkDynamicState> dynamic_states;

	bool enable_color_attachment{true};

	VkPipelineLayout layout;
	VkPipeline pipeline;

protected:
	void createPipeline();

	void createRayTracingPipeline();

private:
	VkPipelineLayoutCreateInfo pipeline_layout{};
	std::vector<VkDescriptorSetLayout> descriptor_layouts{};
	std::vector<VkPushConstantRange> push_constants{};

	std::vector<ShaderManager> shader_managers;
	std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

	std::vector<VkVertexInputAttributeDescription> attribute_descriptions{};

	VkViewport viewport{};
	VkRect2D scissor{};
	VkPipelineViewportStateCreateInfo viewport_state{};

	VkRenderPass render_pass;
	uint32_t subpass;

	ContextManagerSPtr context_manager_sptr;

	PipelineType type;

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups{};
};
