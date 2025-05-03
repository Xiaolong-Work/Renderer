#pragma once

#include <vulkan_render_base.h>

class VulkanRasterDeferredRenderer : public VulkanRendererBase
{
public:
	VulkanRasterDeferredRenderer()
	{
		this->init();
	}

	~VulkanRasterDeferredRenderer()
	{
		clear();
	}

	void init()
	{
		VulkanRendererBase::init();

		auto context_manager_sptr = std::make_shared<ContextManager>(this->context_manager);
		auto command_manager_sptr = std::make_shared<CommandManager>(this->command_manager);
		auto swap_chain_manager_sptr = std::make_shared<SwapChainManager>(this->swap_chain_manager);

		this->geometry_buffer_manager = GeometryBufferManager(context_manager_sptr, command_manager_sptr);
		this->geometry_buffer_manager.setExtent(this->swap_chain_manager.extent);
		this->geometry_buffer_manager.init();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			this->gbuffer_descriptor_managers[i] = DescriptorManager(context_manager_sptr);
			this->light_descriptor_managers[i] = DescriptorManager(context_manager_sptr);
			this->uniform_buffer_managers[i] = UniformBufferManager(context_manager_sptr, command_manager_sptr);
		}

		this->gbuffer_pipeline_manager = PipelineManager(context_manager_sptr);
		this->light_pipeline_manager = PipelineManager(context_manager_sptr);

		this->render_pass_manager = RenderPassManager(
			context_manager_sptr, swap_chain_manager_sptr, command_manager_sptr, RenderPassType::Custom);
	}

	void clear()
	{
		this->render_pass_manager.clear();

		this->light_pipeline_manager.clear();

		this->gbuffer_pipeline_manager.clear();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			this->gbuffer_descriptor_managers[i].clear();
			this->light_descriptor_managers[i].clear();
		}

		this->geometry_buffer_manager.clear();
	}

	void setData(const Scene& scene)
	{
		VulkanRendererBase::setData(scene);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			this->uniform_buffer_managers[i].setData(&this->ubo, sizeof(UBOMVP), 1);
			this->uniform_buffer_managers[i].init();

			this->setupLightDescriptorSet(i);
			this->setupGBufferDescriptorSet(i);
		}

		this->setupRenderPass();

		this->setupGBufferPipeline();
		this->setupLightPipeline();
	}

protected:
	void setupGBufferSubpass()
	{
		AttachmentReference reference{};
		reference.color.push_back(VkAttachmentReference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
		reference.color.push_back(VkAttachmentReference{1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
		reference.color.push_back(VkAttachmentReference{2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
		reference.color.push_back(VkAttachmentReference{3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependency.dstStageMask =
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		this->render_pass_manager.addDependency(dependency);
		this->render_pass_manager.addSubpass(reference);
	}

	void setupGBufferDescriptorSet(const int index)
	{
		/* Camera mvp matrices */
		this->gbuffer_descriptor_managers[index].addDescriptor(
			this->uniform_buffer_managers[index].getDescriptor(0, VK_SHADER_STAGE_VERTEX_BIT));
		/* Object model matrix */
		this->gbuffer_descriptor_managers[index].addDescriptor(
			this->model_matrix_manager.getDescriptor(1, VK_SHADER_STAGE_VERTEX_BIT));
		/* Texture sampler binding */
		this->gbuffer_descriptor_managers[index].addDescriptor(
			this->texture_manager.getDescriptor(2, VK_SHADER_STAGE_FRAGMENT_BIT));
		/* Material index bingding */
		this->gbuffer_descriptor_managers[index].addDescriptor(
			this->material_index_manager.getDescriptor(3, VK_SHADER_STAGE_FRAGMENT_BIT));
		/* Material data binding */
		this->gbuffer_descriptor_managers[index].addDescriptor(
			this->material_ssbo_manager.getDescriptor(4, VK_SHADER_STAGE_FRAGMENT_BIT));

		this->gbuffer_descriptor_managers[index].init();
	}

	void setupGBufferPipeline()
	{
		std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		this->gbuffer_pipeline_manager.dynamic_states = dynamicStates;

		this->gbuffer_pipeline_manager.addShaderStage("deferred_geometry_buffer_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		this->gbuffer_pipeline_manager.addShaderStage("deferred_geometry_buffer_frag.spv",
													  VK_SHADER_STAGE_FRAGMENT_BIT);

		this->gbuffer_pipeline_manager.setDefaultFixedState();
		this->gbuffer_pipeline_manager.setExtent(this->swap_chain_manager.extent);
		this->gbuffer_pipeline_manager.setRenderPass(this->render_pass_manager.pass, 0);
		this->gbuffer_pipeline_manager.setVertexInput(0b1111);
		std::vector<VkDescriptorSetLayout> layout = {this->gbuffer_descriptor_managers[0].layout};
		this->gbuffer_pipeline_manager.setLayout(layout);

		for (size_t i = 1; i < 4; i++)
		{
			this->gbuffer_pipeline_manager.color_blend_attachments.push_back(
				this->gbuffer_pipeline_manager.color_blend_attachments[0]);
		}

		this->gbuffer_pipeline_manager.color_blending.attachmentCount =
			static_cast<uint32_t>(this->gbuffer_pipeline_manager.color_blend_attachments.size());
		this->gbuffer_pipeline_manager.color_blending.pAttachments =
			this->gbuffer_pipeline_manager.color_blend_attachments.data();

		this->gbuffer_pipeline_manager.init();
	}

	void setupLightSubpass()
	{
		AttachmentReference reference{};
		reference.color.push_back(VkAttachmentReference{4, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
		reference.input.push_back(VkAttachmentReference{0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
		reference.input.push_back(VkAttachmentReference{1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
		reference.input.push_back(VkAttachmentReference{2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
		reference.input.push_back(VkAttachmentReference{3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

		VkSubpassDependency dependency{};
		dependency.srcSubpass = 0;
		dependency.dstSubpass = 1;
		dependency.srcStageMask =
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		this->render_pass_manager.addDependency(dependency);
		this->render_pass_manager.addSubpass(reference);
	}

	void setupLightDescriptorSet(const int index)
	{
		/* ========== Layout binding infomation ========== */
		/* Input attachment layout binding */
		auto bindings = this->geometry_buffer_manager.getLayoutBindings(0, VK_SHADER_STAGE_FRAGMENT_BIT);
		for (auto& binding : bindings)
		{
			this->light_descriptor_managers[index].addLayoutBinding(binding);
		}
		/* Point light shadow map sampler binding */
		VkDescriptorSetLayoutBinding point_light_shadow_map_layout_binding{};
		point_light_shadow_map_layout_binding.binding = 4;
		point_light_shadow_map_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		point_light_shadow_map_layout_binding.descriptorCount = 1;
		point_light_shadow_map_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		point_light_shadow_map_layout_binding.pImmutableSamplers = nullptr;
		this->light_descriptor_managers[index].addLayoutBinding(point_light_shadow_map_layout_binding);
		/* Point light data binding */
		this->light_descriptor_managers[index].addLayoutBinding(
			this->shadow_map_manager.point_light_manager.getLayoutBinding(5, VK_SHADER_STAGE_FRAGMENT_BIT));

		/* ========== Pool size infomation ========== */
		/* Input attachment pool size */
		this->light_descriptor_managers[index].addPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 4);
		/* Image sampler size */
		this->light_descriptor_managers[index].addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);
		/* Storage buffer pool size */
		this->light_descriptor_managers[index].addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1);

		/* ========== Write Descriptor Set ========== */
		/* Input attachment write information */
		auto writes = this->geometry_buffer_manager.getWriteInformations(0);
		for (auto& write : writes)
		{
			this->light_descriptor_managers[index].addWrite(write);
		}
		if (!this->shadow_map_manager.point_lights.empty())
		{
			/* Point light shadow map sampler write information */
			VkDescriptorImageInfo shadow_information;
			shadow_information.imageView = this->shadow_map_manager.point_light_shadow_map_manager.view;
			shadow_information.sampler = this->shadow_map_manager.point_light_shadow_map_manager.sampler;
			shadow_information.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			VkWriteDescriptorSet shadow_write{};
			shadow_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			shadow_write.pNext = nullptr;
			shadow_write.dstSet = this->light_descriptor_managers[index].set;
			shadow_write.dstBinding = 4;
			shadow_write.dstArrayElement = 0;
			shadow_write.descriptorCount = 1;
			shadow_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			shadow_write.pBufferInfo = nullptr;
			shadow_write.pImageInfo = &shadow_information;
			shadow_write.pTexelBufferView = nullptr;
			this->light_descriptor_managers[index].addWrite(shadow_write);

			/* ========== Write Descriptor Set ========== */
			/* Point light data write information */
			this->light_descriptor_managers[index].addWrite(
				this->shadow_map_manager.point_light_manager.getWriteInformation(5));
		}

		this->light_descriptor_managers[index].init();
	}

	void setupLightPipeline()
	{
		std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		this->light_pipeline_manager.dynamic_states = dynamicStates;

		this->light_pipeline_manager.addShaderStage("empty_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		this->light_pipeline_manager.addShaderStage("deferred_light_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		this->light_pipeline_manager.setDefaultFixedState();
		this->light_pipeline_manager.setExtent(this->swap_chain_manager.extent);
		this->light_pipeline_manager.setRenderPass(this->render_pass_manager.pass, 1);
		this->light_pipeline_manager.setVertexInput(0);
		std::vector<VkDescriptorSetLayout> layout = {this->light_descriptor_managers[0].layout};
		this->light_pipeline_manager.setLayout(layout);
		this->light_pipeline_manager.init();
	}

	void setupRenderPass()
	{
		auto formats = this->geometry_buffer_manager.getFormats();
		for (auto format : formats)
		{
			this->render_pass_manager.addColorAttachment(format,
														 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
														 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
														 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		this->render_pass_manager.addColorAttachment(
			this->swap_chain_manager.format, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		this->render_pass_manager.setDefaultDepthAttachment();

		this->setupGBufferSubpass();
		this->setupLightSubpass();

		this->render_pass_manager.views.resize(this->swap_chain_manager.views.size());
		for (size_t i = 0; i < this->swap_chain_manager.views.size(); i++)
		{
			this->render_pass_manager.views[i] = this->geometry_buffer_manager.getViews();
			this->render_pass_manager.views[i].push_back(this->swap_chain_manager.views[i]);
		}

		this->render_pass_manager.init();
	}

	void updateUniformBufferObjects(const int index) override
	{
		this->uniform_buffer_managers[index].update(&this->ubo);
	}

	void recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index) override
	{
		/* Clear Value */
		std::array<VkClearValue, 6> clear_values{};
		clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clear_values[1].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clear_values[2].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clear_values[3].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clear_values[4].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clear_values[5].depthStencil = {1.0f, 0};

		/* Dynamic setting of viewport */
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(this->swap_chain_manager.extent.width);
		viewport.height = static_cast<float>(this->swap_chain_manager.extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		/* Dynamic setting of scissor rectangle */
		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = this->swap_chain_manager.extent;

		VkCommandBufferBeginInfo command_begin{};
		command_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		command_begin.pNext = nullptr;
		command_begin.flags = 0;
		command_begin.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(command_buffer, &command_begin) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo render_pass_begin{};
		render_pass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_begin.renderPass = this->render_pass_manager.pass;
		render_pass_begin.framebuffer = this->render_pass_manager.buffers[image_index];
		render_pass_begin.renderArea.offset = {0, 0};
		render_pass_begin.renderArea.extent = this->swap_chain_manager.extent;
		render_pass_begin.clearValueCount = static_cast<uint32_t>(clear_values.size());
		render_pass_begin.pClearValues = clear_values.data();

		vkCmdBeginRenderPass(command_buffer, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

		/* ========== Geometry Processing Subpass ========== */
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->gbuffer_pipeline_manager.pipeline);

		vkCmdSetViewport(command_buffer, 0, 1, &viewport);

		vkCmdSetScissor(command_buffer, 0, 1, &scissor);

		VkDeviceSize offsets = 0;
		vkCmdBindVertexBuffers(command_buffer, 0, 1, &this->vertex_buffer_manager.buffer, &offsets);

		vkCmdBindIndexBuffer(command_buffer, this->index_buffer_manager.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(command_buffer,
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								this->gbuffer_pipeline_manager.layout,
								0,
								1,
								&this->gbuffer_descriptor_managers[current_frame].set,
								0,
								nullptr);

		vkCmdDrawIndexedIndirect(command_buffer,
								 this->indirect_buffer_manager.buffer,
								 0,
								 static_cast<uint32_t>(this->indirect_buffer_manager.commands.size()),
								 sizeof(VkDrawIndexedIndirectCommand));

		/* ========== Lighting calculation subpass ========== */
		vkCmdNextSubpass(command_buffer, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->light_pipeline_manager.pipeline);

		vkCmdSetViewport(command_buffer, 0, 1, &viewport);

		vkCmdSetScissor(command_buffer, 0, 1, &scissor);

		vkCmdBindDescriptorSets(command_buffer,
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								this->light_pipeline_manager.layout,
								0,
								1,
								&this->light_descriptor_managers[current_frame].set,
								0,
								nullptr);

		vkCmdDraw(command_buffer, 6, 1, 0, 0);

		vkCmdEndRenderPass(command_buffer);

		if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer!");
		}
	}

private:
	GeometryBufferManager geometry_buffer_manager{};

	PipelineManager gbuffer_pipeline_manager{};

	PipelineManager light_pipeline_manager{};

	std::array<UniformBufferManager, MAX_FRAMES_IN_FLIGHT> uniform_buffer_managers{};

	std::array<DescriptorManager, MAX_FRAMES_IN_FLIGHT> gbuffer_descriptor_managers{};

	std::array<DescriptorManager, MAX_FRAMES_IN_FLIGHT> light_descriptor_managers{};
};
