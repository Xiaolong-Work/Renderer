#pragma once

#include <stdexcept>
#include <vector>

#include <buffer_manager.h>
#include <context_manager.h>
#include <shader_manager.h>

class PipelineManager
{
public:
	PipelineManager() = default;
	PipelineManager(const ContextManagerSPtr& context_manager_sptr);

	void init();
	void clear();

	void addShaderStage(const std::string& name, const VkShaderStageFlagBits stage);

	void setDefaultFixedState();

	void setExtent(const VkExtent2D extent);

	void setRenderPass(const VkRenderPass render_pass, const uint32_t subpass = 0);

	void setVertexInput(const uint32_t mask)
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

	std::vector<VkVertexInputAttributeDescription> attribute_descriptions{};

	void setDescriptorSetLayout(const std::vector<VkDescriptorSetLayout>& descriptor_layouts,
								const std::vector<VkPushConstantRange>& push_constants = {});

	/* Optional Values */
	void* next{nullptr};
	VkPipelineInputAssemblyStateCreateInfo input_assembly{};
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	VkPipelineMultisampleStateCreateInfo multisample{};
	VkPipelineDepthStencilStateCreateInfo depth_stencil{};
	std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments{};
	VkPipelineColorBlendStateCreateInfo color_blending{};
	std::vector<VkDynamicState> dynamic_states;

	bool enable_vertex_inpute{true};
	bool enable_color_attachment{true};

	VkPipelineLayout layout;
	VkPipeline pipeline;

protected:
	void createPipeline();

private:
	VkPipelineLayoutCreateInfo pipeline_layout{};
	std::vector<VkDescriptorSetLayout> descriptor_layouts{};
	std::vector<VkPushConstantRange> push_constants{};

	std::vector<ShaderManager> shader_managers;
	std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

	VkViewport viewport{};
	VkRect2D scissor{};
	VkPipelineViewportStateCreateInfo viewport_state{};

	VkRenderPass render_pass;
	uint32_t subpass;

	ContextManagerSPtr context_manager_sptr;
};
