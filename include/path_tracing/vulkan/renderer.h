#pragma once

#include <buffer_manager.h>
#include <command_manager.h>
#include <descriptor_manager.h>
#include <fstream>
#include <image_manager.h>
#include <iostream>
#include <pipeline_manager.h>
#include <path_tracing_scene.h>
#include <shader_manager.h>
#include <ssbo_buffer_manager.h>
#include <swap_chain_manager.h>
#include <texture_manager.h>

#define MAX_FRAMES_IN_FLIGHT 2
class VulkanPathTracingRender
{
public:
	~VulkanPathTracingRender()
	{
		clear();
	}

	int spp;
	void setSpp(int spp)
	{
		this->spp = spp;
	}

	VkPipeline computePipeline;
	VkPipelineLayout computePipelineLayout;
	ShaderManager computeShaderManager;

	void init(const PathTracingScene& scene)
	{
		this->context_manager.setExtent(VkExtent2D{scene.camera.width, scene.camera.height});
		this->context_manager.init();
		auto pContentManager = std::make_shared<ContextManager>(this->context_manager);

		this->command_manager = CommandManager(pContentManager);
		this->command_manager.init();
		auto pCommandManager = std::make_shared<CommandManager>(this->command_manager);

		this->swap_chain_manager = SwapChainManager(pContentManager, pCommandManager);
		this->swap_chain_manager.init();

		this->vertex_shader_manager = ShaderManager(pContentManager);
		this->vertex_shader_manager.setShaderName("path_tracing_vert.spv");
		this->vertex_shader_manager.init();

		this->fragment_shader_manager = ShaderManager(pContentManager);
		this->fragment_shader_manager.setShaderName("path_tracing_frag.spv");
		this->fragment_shader_manager.init();

		this->computeShaderManager = ShaderManager(pContentManager);
		this->computeShaderManager.setShaderName("path_tracing_comp.spv");
		this->computeShaderManager.init();

		this->bufferManager = SSBOBufferManager(pContentManager, pCommandManager);
		this->bufferManager.setData(scene, this->spp);
		this->bufferManager.init();

		this->storageImageManager = StorageImageManager(pContentManager, pCommandManager);
		this->storageImageManager.setExtent(VkExtent2D{scene.camera.width, scene.camera.height});
		this->storageImageManager.init();

		this->texture_manager = TextureManager(pContentManager, pCommandManager);
		this->texture_manager.init();

		for (auto& texture_path : this->bufferManager.texture_paths)
		{
			this->texture_manager.createTexture(texture_path);
		}

		createDescriptorSetLayout();
		createDescriptorPool();
		createDescriptorSets();

		createComputePipeline();

		createRenderPass();
		createFramebuffers();

		this->graphics_pipeline_manager = PipelineManager(pContentManager);
		this->setupGraphicsPipelines();
		this->graphics_pipeline_manager.init();

		createSyncObjects();
	}

	void setupGraphicsPipelines()
	{
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.pNext = nullptr;
		vertShaderStageInfo.flags = 0;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertex_shader_manager.module;
		vertShaderStageInfo.pName = "main";
		vertShaderStageInfo.pSpecializationInfo = nullptr;

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.pNext = nullptr;
		fragShaderStageInfo.flags = 0;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragment_shader_manager.module;
		fragShaderStageInfo.pName = "main";
		vertShaderStageInfo.pSpecializationInfo = nullptr;

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swap_chain_manager.extent.width;
		viewport.height = (float)swap_chain_manager.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swap_chain_manager.extent;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		graphics_pipeline_manager.setRequiredValue(shaderStages, viewport, scissor, pipelineLayoutInfo, renderPass);
		graphics_pipeline_manager.enableVertexInpute = false;
	}

	void createRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.flags = 0;
		colorAttachment.format = swap_chain_manager.format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		/*VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		dependency.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;*/
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask =
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask =
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency.dependencyFlags = 0;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(context_manager.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create render pass!");
		}
	}

	void createFramebuffers()
	{
		this->framebuffers.resize(this->swap_chain_manager.imageViews.size());

		for (size_t i = 0; i < this->swap_chain_manager.imageViews.size(); i++)
		{
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.pNext = nullptr;
			framebufferInfo.flags = 0;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &swap_chain_manager.imageViews[i];
			framebufferInfo.width = swap_chain_manager.extent.width;
			framebufferInfo.height = swap_chain_manager.extent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(context_manager.device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create framebuffer!");
			}
		}
	}

	std::vector<VkFramebuffer> framebuffers;
	VkRenderPass renderPass;

	ShaderManager vertex_shader_manager{};
	ShaderManager fragment_shader_manager{};

	VkPipeline graphicsPipeline;

	PipelineManager graphics_pipeline_manager{};

	void createComputePipeline()
	{
		VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
		computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		computeShaderStageInfo.pNext = nullptr;
		computeShaderStageInfo.flags = 0;
		computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		computeShaderStageInfo.module = this->computeShaderManager.module;
		computeShaderStageInfo.pName = "main";
		computeShaderStageInfo.pSpecializationInfo = nullptr;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

		if (vkCreatePipelineLayout(context_manager.device, &pipelineLayoutInfo, nullptr, &computePipelineLayout) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create compute pipeline layout!");
		}

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = computePipelineLayout;
		pipelineInfo.stage = computeShaderStageInfo;

		if (vkCreateComputePipelines(
				context_manager.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create compute pipeline!");
		}
	}

	void draw()
	{
		auto commandBuffer = command_manager.beginComputeCommands();

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
		vkCmdBindDescriptorSets(
			commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &descriptorSet, 0, 0);

		vkCmdDispatch(commandBuffer, this->bufferManager.scene.width / 8, this->bufferManager.scene.height / 8, 1);

		command_manager.endComputeCommands(commandBuffer);

		while (!glfwWindowShouldClose(context_manager.window))
		{
			glfwPollEvents();
			present();
		}

		vkDeviceWaitIdle(context_manager.device);
	}

	void createSyncObjects()
	{
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreInfo.pNext = nullptr;
		semaphoreInfo.flags = 0;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.pNext = nullptr;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(context_manager.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) !=
					VK_SUCCESS ||
				vkCreateSemaphore(context_manager.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) !=
					VK_SUCCESS ||
				vkCreateFence(context_manager.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			{

				throw std::runtime_error("Failed to create synchronization objects for a frame!");
			}
		}
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
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = framebuffers[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swap_chain_manager.extent;
		VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->graphics_pipeline_manager.pipeline);

		vkCmdBindDescriptorSets(commandBuffer,
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								this->graphics_pipeline_manager.layout,
								0,
								1,
								&descriptorSet,
								0,
								nullptr);

		vkCmdDraw(commandBuffer, 6, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer!");
		}
	}

	/* 同步量 */
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	uint32_t currentFrame = 0;
	void present()
	{
		vkWaitForFences(context_manager.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		/* 获取交换链图像 */
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(context_manager.device,
												this->swap_chain_manager.swapChain,
												UINT64_MAX,
												imageAvailableSemaphores[currentFrame],
												VK_NULL_HANDLE,
												&imageIndex);

		/* 交换链与窗口表面不兼容 */
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			// recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		vkResetFences(context_manager.device, 1, &inFlightFences[currentFrame]);

		/* 记录命令缓冲区 */
		vkResetCommandBuffer(command_manager.graphicsCommandBuffers[currentFrame], 0);
		recordCommandBuffer(command_manager.graphicsCommandBuffers[currentFrame], imageIndex);

		/* 提交命令缓冲区 */
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;

		VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &command_manager.graphicsCommandBuffers[currentFrame];
		VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(context_manager.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to submit draw command buffer!");
		}

		/* 进行呈现 */
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = {this->swap_chain_manager.swapChain};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		result = vkQueuePresentKHR(this->context_manager.presentQueue, &presentInfo);

		/*if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
		{
			framebufferResized = false;
			recreateSwapChain();
		}
		else */
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present swap chain image!");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	VkDescriptorSetLayout descriptorSetLayout;
	void createDescriptorSetLayout()
	{
		std::array<VkDescriptorSetLayoutBinding, 11> layoutBindings{};

		/* Used for compute shader data input buffer */
		for (size_t i = 0; i < 8; i++)
		{
			layoutBindings[i].binding = i;
			layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			layoutBindings[i].descriptorCount = 1;
			layoutBindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			layoutBindings[i].pImmutableSamplers = nullptr;
		}

		/* Used for compute shader result output images */
		layoutBindings[8].binding = 8;
		layoutBindings[8].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		layoutBindings[8].descriptorCount = 1;
		layoutBindings[8].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		layoutBindings[8].pImmutableSamplers = nullptr;

		/* Used For fragment shader texture input */
		layoutBindings[9].binding = 9;
		layoutBindings[9].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindings[9].descriptorCount = 1;
		layoutBindings[9].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBindings[9].pImmutableSamplers = nullptr;

		/* Used For compute shader texture input */
		layoutBindings[10].binding = 10;
		layoutBindings[10].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindings[10].descriptorCount = static_cast<uint32_t>(this->texture_manager.imageViews.size());
		layoutBindings[10].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		layoutBindings[10].pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.pNext = nullptr;
		layoutInfo.flags = 0;
		layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
		layoutInfo.pBindings = layoutBindings.data();

		if (vkCreateDescriptorSetLayout(context_manager.device, &layoutInfo, nullptr, &this->descriptorSetLayout) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create descriptor set layout!");
		}
	}

	VkDescriptorPool descriptorPool;
	void createDescriptorPool()
	{
		std::array<VkDescriptorPoolSize, 3> poolSizes;
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[0].descriptorCount = 8;

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSizes[1].descriptorCount = 1;

		poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[2].descriptorCount = 2;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.pNext = nullptr;
		poolInfo.flags = 0;
		poolInfo.maxSets = 1;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();

		if (vkCreateDescriptorPool(context_manager.device, &poolInfo, nullptr, &this->descriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create descriptor pool!");
		}
	}

	VkDescriptorSet descriptorSet;
	void createDescriptorSets()
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.descriptorPool = this->descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &this->descriptorSetLayout;

		if (vkAllocateDescriptorSets(context_manager.device, &allocInfo, &this->descriptorSet) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate descriptor sets!");
		}

		std::array<VkDescriptorBufferInfo, 8> bufferInfo{};
		for (size_t i = 0; i < 8; i++)
		{
			bufferInfo[i].buffer = this->bufferManager.buffers[i];
			bufferInfo[i].offset = 0;
			bufferInfo[i].range = VK_WHOLE_SIZE;
		}

		std::array<VkDescriptorImageInfo, 2> imageInfo{};
		imageInfo[0].imageView = storageImageManager.imageView;
		imageInfo[0].sampler = VK_NULL_HANDLE;
		imageInfo[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		imageInfo[1].imageView = storageImageManager.imageView;
		imageInfo[1].sampler = this->texture_manager.sampler;
		imageInfo[1].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		std::vector<VkDescriptorImageInfo> textureInfo;
		textureInfo.resize(this->texture_manager.imageViews.size());
		for (size_t i = 0; i < this->texture_manager.imageViews.size(); i++)
		{
			textureInfo[i].imageView = texture_manager.imageViews[i];
			textureInfo[i].sampler = texture_manager.sampler;
			textureInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		std::array<VkWriteDescriptorSet, 11> descriptorWrites{};
		for (size_t i = 0; i < 11; i++)
		{
			descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[i].pNext = nullptr;
			descriptorWrites[i].dstSet = this->descriptorSet;
			descriptorWrites[i].dstBinding = static_cast<uint32_t>(i);
			descriptorWrites[i].dstArrayElement = 0;
			descriptorWrites[i].descriptorCount = 1;
		}

		for (size_t i = 0; i < 8; i++)
		{
			descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[i].pBufferInfo = &bufferInfo[i];
			descriptorWrites[i].pImageInfo = nullptr;
			descriptorWrites[i].pTexelBufferView = nullptr;
		}

		descriptorWrites[8].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptorWrites[8].pBufferInfo = nullptr;
		descriptorWrites[8].pImageInfo = &imageInfo[0];
		descriptorWrites[8].pTexelBufferView = nullptr;

		descriptorWrites[9].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[9].pBufferInfo = nullptr;
		descriptorWrites[9].pImageInfo = &imageInfo[1];
		descriptorWrites[9].pTexelBufferView = nullptr;

		descriptorWrites[10].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[10].pBufferInfo = nullptr;
		descriptorWrites[10].descriptorCount = static_cast<uint32_t> (textureInfo.size());
		descriptorWrites[10].pImageInfo = textureInfo.data();
		descriptorWrites[10].pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(
			context_manager.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	StorageImageManager storageImageManager;

	void saveResult();

	void clear()
	{

		this->texture_manager.clear();

		for (auto framebuffer : framebuffers)
		{
			vkDestroyFramebuffer(context_manager.device, framebuffer, nullptr);
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(context_manager.device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(context_manager.device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(context_manager.device, inFlightFences[i], nullptr);
		}

		this->graphics_pipeline_manager.clear();

		vkDestroyRenderPass(context_manager.device, renderPass, nullptr);

		this->swap_chain_manager.clear();

		this->command_manager.clear();

		this->vertex_shader_manager.clear();
		this->fragment_shader_manager.clear();

		this->context_manager.clear();
	}

	ContextManager context_manager{};
	CommandManager command_manager{};
	SSBOBufferManager bufferManager{};
	TextureManager texture_manager{};
	SwapChainManager swap_chain_manager;
};