#pragma once

#include <buffer_manager.h>
#include <command_manager.h>
#include <descriptor_manager.h>
#include <image_manager.h>
#include <pipeline_manager.h>
#include <render_pass_manager.h>
#include <shader_manager.h>
#include <swap_chain_manager.h>
#include <texture_manager.h>

#include <utils.h>;

class DrawFrame
{
public:
	DrawFrame();
	~DrawFrame();

	void init();
	void clear();

	virtual void draw();
	virtual void render() = 0;

	void setupDescriptor()
	{
		/* Layout bingding */
		VkDescriptorSetLayoutBinding sampler_layout_binding{};
		sampler_layout_binding.binding = 0;
		sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_layout_binding.descriptorCount = 1;
		sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		sampler_layout_binding.pImmutableSamplers = nullptr;
		this->descriptor_manager.addLayoutBinding(sampler_layout_binding);

		/* Pool size */
		this->descriptor_manager.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);

		this->descriptor_manager.init();

		/* Write Descriptor Set  */
		VkDescriptorImageInfo image_infomation{};
		image_infomation.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_infomation.imageView = texture_manager.views[0];
		image_infomation.sampler = texture_manager.samplers[0];
		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.pNext = nullptr;
		write.dstSet = this->descriptor_manager.set;
		write.dstBinding = 0;
		write.dstArrayElement = 0;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.pBufferInfo = nullptr;
		write.pImageInfo = &image_infomation;
		write.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(content_manager.device, static_cast<uint32_t>(1), &write, 0, nullptr);
	}

	void setupGraphicsPipelines()
	{
		this->pipeline_manager.addShaderStage("draw_single_frame_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		this->pipeline_manager.addShaderStage("draw_single_frame_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		this->pipeline_manager.setDefaultFixedState();
		this->pipeline_manager.setExtent(swap_chain_manager.extent);
		std::vector<VkDescriptorSetLayout> layout = {descriptor_manager.layout};
		this->pipeline_manager.setLayout(layout);
		this->pipeline_manager.setRenderPass(render_pass_manager.pass, 0);
		this->pipeline_manager.setVertexInput(0b0000);

		this->pipeline_manager.init();
	}

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = render_pass_manager.pass;
		renderPassInfo.framebuffer = render_pass_manager.buffers[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swap_chain_manager.extent;
		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clearValues[1].depthStencil = {1.0f, 0};
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline_manager.pipeline);

		vkCmdBindDescriptorSets(commandBuffer,
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								this->pipeline_manager.layout,
								0,
								1,
								&this->descriptor_manager.set,
								0,
								nullptr);

		vkCmdDraw(commandBuffer, 6, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer!");
		}
	}

	void createSyncObjects()
	{
		VkSemaphoreCreateInfo semaphore_information{};
		semaphore_information.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphore_information.pNext = nullptr;
		semaphore_information.flags = 0;

		VkFenceCreateInfo fence_information{};
		fence_information.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_information.pNext = nullptr;
		fence_information.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateSemaphore(content_manager.device, &semaphore_information, nullptr, &image_available) !=
				VK_SUCCESS ||
			vkCreateSemaphore(content_manager.device, &semaphore_information, nullptr, &render_finished) !=
				VK_SUCCESS ||
			vkCreateFence(content_manager.device, &fence_information, nullptr, &inFlightFences) != VK_SUCCESS)
		{

			throw std::runtime_error("Failed to create synchronization objects for a frame!");
		}
	}

	void present()
	{
		vkWaitForFences(content_manager.device, 1, &inFlightFences, VK_TRUE, UINT64_MAX);

		/* 获取交换链图像 */
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(content_manager.device,
												this->swap_chain_manager.swap_chain,
												UINT64_MAX,
												image_available,
												VK_NULL_HANDLE,
												&imageIndex);
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		vkResetFences(content_manager.device, 1, &inFlightFences);

		/* 记录命令缓冲区 */
		vkResetCommandBuffer(command_manager.graphicsCommandBuffers[0], 0);
		recordCommandBuffer(command_manager.graphicsCommandBuffers[0], imageIndex);

		/* 提交命令缓冲区 */
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;

		VkSemaphore waitSemaphores[] = {image_available};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &command_manager.graphicsCommandBuffers[0];
		VkSemaphore signalSemaphores[] = {render_finished};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(content_manager.graphics_queue, 1, &submitInfo, inFlightFences) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to submit draw command buffer!");
		}

		/* 进行呈现 */
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = {this->swap_chain_manager.swap_chain};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		if (vkQueuePresentKHR(this->content_manager.present_queue, &presentInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present swap chain image!");
		}

		this->texture_manager.transformLayout(this->texture_manager.images[0],
											  VK_FORMAT_R32G32B32A32_SFLOAT,
											  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
											  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	}

	/* 同步量 */
	VkSemaphore image_available;
	VkSemaphore render_finished;
	VkFence inFlightFences;

protected:
	ContextManager content_manager{};

	CommandManager command_manager{};

	SwapChainManager swap_chain_manager{};

	RenderPassManager render_pass_manager{};

	ShaderManager vertex_shader_manager{};

	ShaderManager fragment_shader_manager{};

	TextureManager texture_manager{};

	StagingBufferManager buffer_manager{};

	DescriptorManager descriptor_manager{};

	PipelineManager pipeline_manager{};

	bool window_resized{false};
};