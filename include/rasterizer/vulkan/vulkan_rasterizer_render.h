#pragma once

#include <geometry_buffer_manager.h>
#include <vulkan_render_base.h>

#include <scene.h>

class VulkanRasterRenderer : public VulkanRendererBase
{
public:
	VulkanRasterRenderer()
	{
		init();
	}

	~VulkanRasterRenderer()
	{
		clear();
	}

	void setData(const Scene& scene)
	{
		VulkanRendererBase::setData(scene);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			this->uniform_buffer_managers[i].setData(&this->ubo, sizeof(UBOMVP), 1);
			this->uniform_buffer_managers[i].init();
			this->setupDescriptor(i);
		}

		this->render_pass_manager.init();

		this->setupGraphicsPipelines();
	}

protected:
	void init()
	{
		auto context_manager_sptr = std::make_shared<ContextManager>(this->context_manager);
		auto command_manager_sptr = std::make_shared<CommandManager>(this->command_manager);
		auto swap_chain_manager_sptr = std::make_shared<SwapChainManager>(this->swap_chain_manager);

		this->render_pass_manager =
			RenderPassManager(context_manager_sptr, swap_chain_manager_sptr, command_manager_sptr);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			this->uniform_buffer_managers[i] = UniformBufferManager(context_manager_sptr, command_manager_sptr);
			this->descriptor_managers[i] = DescriptorManager(context_manager_sptr);
		}

		this->pipeline_manager = PipelineManager(context_manager_sptr);
	}

	void clear()
	{
		this->pipeline_manager.clear();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			this->uniform_buffer_managers[i].clear();
			this->descriptor_managers[i].clear();
		}
	}

	void setupGraphicsPipelines()
	{
		std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		this->pipeline_manager.dynamic_states = dynamicStates;

		this->pipeline_manager.addShaderStage("rasterize_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		this->pipeline_manager.addShaderStage("rasterize_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		this->pipeline_manager.setDefaultFixedState();
		this->pipeline_manager.setExtent(this->swap_chain_manager.extent);
		this->pipeline_manager.setRenderPass(this->render_pass_manager.pass);
		this->pipeline_manager.setVertexInput(0b1111);
		std::vector<VkDescriptorSetLayout> layout = {this->descriptor_managers[0].layout};
		this->pipeline_manager.setDescriptorSetLayout(layout);

		this->pipeline_manager.init();
	}

	void setupDescriptor(const int index)
	{
		/* ========== Layout binding infomation ========== */
		/* MVP UBO binding */
		this->descriptor_managers[index].addLayoutBinding(
			this->uniform_buffer_managers[index].getLayoutBinding(0, VK_SHADER_STAGE_VERTEX_BIT));

		this->descriptor_managers[index].addLayoutBinding(
			this->model_matrix_manager.getLayoutBinding(1, VK_SHADER_STAGE_VERTEX_BIT));

		/* Texture sampler binding */
		this->descriptor_managers[index].addLayoutBinding(
			this->texture_manager.getLayoutBinding(2, VK_SHADER_STAGE_FRAGMENT_BIT));

		/* Point light shadow map sampler binding */
		VkDescriptorSetLayoutBinding point_light_shadow_map_layout_binding{};
		point_light_shadow_map_layout_binding.binding = 3;
		point_light_shadow_map_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		point_light_shadow_map_layout_binding.descriptorCount = 1;
		point_light_shadow_map_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		point_light_shadow_map_layout_binding.pImmutableSamplers = nullptr;
		this->descriptor_managers[index].addLayoutBinding(point_light_shadow_map_layout_binding);

		/* Point light data binding */
		this->descriptor_managers[index].addLayoutBinding(
			this->shadow_map_manager.point_light_manager.getLayoutBinding(4, VK_SHADER_STAGE_FRAGMENT_BIT));

		/* Material index bingding */
		this->descriptor_managers[index].addLayoutBinding(
			this->material_index_manager.getLayoutBinding(5, VK_SHADER_STAGE_FRAGMENT_BIT));

		/* Material data binding */
		this->descriptor_managers[index].addLayoutBinding(
			this->material_ssbo_manager.getLayoutBinding(6, VK_SHADER_STAGE_FRAGMENT_BIT));

		/* Point light mvp binding */
		this->descriptor_managers[index].addLayoutBinding(
			this->shadow_map_manager.mvp_ssbo_manager.getLayoutBinding(7, VK_SHADER_STAGE_FRAGMENT_BIT));

		/* ========== Pool size infomation ========== */
		/* UBO pool size */
		this->descriptor_managers[index].addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);

		/* Sampler pool size */
		this->descriptor_managers[index].addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2);

		/* SSBO pool size */
		this->descriptor_managers[index].addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5);

		/* ========== Write Descriptor Set ========== */
		std::vector<VkWriteDescriptorSet> descriptor_writes{};

		/* MVP UBO set write information */
		this->descriptor_managers[index].addWrite(this->uniform_buffer_managers[index].getWriteInformation(0));

		this->descriptor_managers[index].addWrite(this->model_matrix_manager.getWriteInformation(1));

		/* Texture sampler write information */
		if (!this->texture_manager.isEmpty())
		{
			this->descriptor_managers[index].addWrite(this->texture_manager.getWriteInformation(2));
		}

		/* Point light shadow map sampler write information */
		if (!this->shadow_map_manager.point_lights.empty())
		{
			VkDescriptorImageInfo shadow_information;
			shadow_information.imageView = this->shadow_map_manager.point_light_shadow_map_manager.view;
			shadow_information.sampler = this->shadow_map_manager.point_light_shadow_map_manager.sampler;
			shadow_information.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			VkWriteDescriptorSet shadow_write{};
			shadow_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			shadow_write.pNext = nullptr;
			shadow_write.dstSet = this->descriptor_managers[index].set;
			shadow_write.dstBinding = 3;
			shadow_write.dstArrayElement = 0;
			shadow_write.descriptorCount = 1;
			shadow_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			shadow_write.pBufferInfo = nullptr;
			shadow_write.pImageInfo = &shadow_information;
			shadow_write.pTexelBufferView = nullptr;
			this->descriptor_managers[index].addWrite(shadow_write);
		}

		/* Point light data write information */
		if (!this->shadow_map_manager.point_lights.empty())
		{
			this->descriptor_managers[index].addWrite(
				this->shadow_map_manager.point_light_manager.getWriteInformation(4));
		}

		/* Material index set write information */
		this->descriptor_managers[index].addWrite(this->material_index_manager.getWriteInformation(5));

		/* Material data set write information */
		this->descriptor_managers[index].addWrite(this->material_ssbo_manager.getWriteInformation(6));

		this->descriptor_managers[index].addWrite(this->shadow_map_manager.mvp_ssbo_manager.getWriteInformation(7));

		this->descriptor_managers[index].init();
	}

	void updateUniformBufferObjects(const int index) override
	{
		this->uniform_buffer_managers[index].update(&this->ubo);
	}

	void recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index) override
	{
		/* Clear Value */
		std::array<VkClearValue, 2> clear_values{};
		clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clear_values[1].depthStencil = {1.0f, 0};

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

		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline_manager.pipeline);

		vkCmdSetViewport(command_buffer, 0, 1, &viewport);

		vkCmdSetScissor(command_buffer, 0, 1, &scissor);

		VkDeviceSize offsets = 0;
		vkCmdBindVertexBuffers(command_buffer, 0, 1, &this->vertex_buffer_manager.buffer, &offsets);

		vkCmdBindIndexBuffer(command_buffer, this->index_buffer_manager.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(command_buffer,
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								this->pipeline_manager.layout,
								0,
								1,
								&this->descriptor_managers[current_frame].set,
								0,
								nullptr);

		vkCmdDrawIndexedIndirect(command_buffer,
								 this->indirect_buffer_manager.buffer,
								 0,
								 static_cast<uint32_t>(this->indirect_buffer_manager.commands.size()),
								 sizeof(VkDrawIndexedIndirectCommand));

		vkCmdEndRenderPass(command_buffer);

		if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer!");
		}
	}

private:
	std::array<UniformBufferManager, MAX_FRAMES_IN_FLIGHT> uniform_buffer_managers{};

	std::array<DescriptorManager, MAX_FRAMES_IN_FLIGHT> descriptor_managers{};

	PipelineManager pipeline_manager{};
};