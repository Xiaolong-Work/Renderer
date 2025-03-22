#pragma once

#include <fstream>
#include <iostream>

#include <buffer_manager.h>
#include <command_manager.h>
#include <descriptor_manager.h>
#include <image_manager.h>
#include <pipeline_manager.h>
#include <shader_manager.h>
#include <ssbo_buffer_manager.h>
#include <swap_chain_manager.h>
#include <texture_manager.h>

#include <tiny_obj_loader.h>

const std::string MODEL_PATH = std::string(ROOT_DIR) + "/models/viking_room/viking_room.obj";
const std::string TEXTURE_PATH = std::string(ROOT_DIR) + "/models/viking_room/viking_room.png";

class VulkanRasterRenderer
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

	void run()
	{
		while (!glfwWindowShouldClose(contentManager.window))
		{
			glfwPollEvents();
			draw();
		}

		vkDeviceWaitIdle(contentManager.device);
	}

protected:
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto self = reinterpret_cast<VulkanRasterRenderer*>(glfwGetWindowUserPointer(window));
		self->framebufferResized = true;
	}

	static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
	{
		auto self = static_cast<VulkanRasterRenderer*>(glfwGetWindowUserPointer(window));

		double differenceX = xpos - self->mousePositionX;
		double differenceY = ypos - self->mousePositionY;

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{
			float angle = sqrt(differenceX * differenceX + differenceY * differenceY) / 3.6f;
			glm::vec3 rotationAxis(differenceY, differenceX, 0.0f);
			rotationAxis = glm::normalize(rotationAxis);
			self->ubo.model = glm::rotate(self->ubo.model, glm::radians(angle), rotationAxis);
		}

		self->mousePositionX = xpos;
		self->mousePositionY = ypos;
		return;
	}

	void initUBO()
	{
		ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.project = glm::perspective(glm::radians(45.0f),
									   (float)swapChainManager.extent.width / (float)swapChainManager.extent.height,
									   0.1f,
									   10.0f);
		ubo.project[1][1] *= -1;

		glfwSetFramebufferSizeCallback(this->contentManager.window, framebufferResizeCallback);
		glfwSetCursorPosCallback(this->contentManager.window, cursorPositionCallback);
		glfwSetWindowUserPointer(this->contentManager.window, this);
	}

	void init()
	{
		this->contentManager.init();
		auto pContentManager = std::make_shared<ContentManager>(this->contentManager);

		this->commandManager = CommandManager(pContentManager);
		this->commandManager.init();
		auto pCommandManager = std::make_shared<CommandManager>(this->commandManager);

		this->swapChainManager = SwapChainManager(pContentManager, pCommandManager);
		this->swapChainManager.init();

		this->depthImageManager = DepthImageManager(pContentManager, pCommandManager);
		this->depthImageManager.setExtent(this->swapChainManager.extent);
		this->depthImageManager.init();

		this->vertexShaderManager = ShaderManager(pContentManager);
		this->vertexShaderManager.setShaderName("vertex.spv");
		this->vertexShaderManager.init();

		this->fragmentShaderManager = ShaderManager(pContentManager);
		this->fragmentShaderManager.setShaderName("fragment.spv");
		this->fragmentShaderManager.init();

		createRenderPass();

		createFramebuffers();

		this->textureManager = TextureManager(pContentManager, pCommandManager);
		this->textureManager.init();
		this->textureManager.createTexture(TEXTURE_PATH);
		auto pTextureManager = std::make_shared<TextureManager>(this->textureManager);

		this->vertexBufferManager = VertexBufferManager(pContentManager, pCommandManager);
		this->indexBufferManager = IndexBufferManager(pContentManager, pCommandManager);
		loadModel();
		this->vertexBufferManager.init();
		this->indexBufferManager.init();

		this->uniformBufferManager = UniformBufferManager(pContentManager, pCommandManager);
		this->uniformBufferManager.init();
		auto pUniformBufferManager = std::make_shared<UniformBufferManager>(this->uniformBufferManager);

		this->descriptorManager = DescriptorManager(pContentManager, pUniformBufferManager, pTextureManager);
		this->descriptorManager.init();

		this->graphicsPipelineManager = PipelineManager(pContentManager);
		this->setupGraphicsPipelines();
		this->graphicsPipelineManager.init();

		createSyncObjects();

		initUBO();
	}

	void loadModel()
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
		{
			throw std::runtime_error(warn + err);
		}

		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				Vertex1 vertex{};

				vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
							  attrib.vertices[3 * index.vertex_index + 1],
							  attrib.vertices[3 * index.vertex_index + 2]};

				vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
								   1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

				vertex.color = {1.0f, 1.0f, 1.0f};

				vertexBufferManager.vertices.push_back(vertex);
				indexBufferManager.indices.push_back(indexBufferManager.indices.size());
			}
		}
	}

	void cleanupSwapChain()
	{
		depthImageManager.clear();

		for (size_t i = 0; i < framebuffers.size(); i++)
		{
			vkDestroyFramebuffer(contentManager.device, framebuffers[i], nullptr);
		}

		this->swapChainManager.clear();
	}

	void recreateSwapChain()
	{
		int width = 0, height = 0;

		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(contentManager.window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(contentManager.device);

		cleanupSwapChain();
		this->swapChainManager.init();
		depthImageManager.setExtent(this->swapChainManager.extent);
		depthImageManager.init();
		createFramebuffers();
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
			if (vkCreateSemaphore(contentManager.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) !=
					VK_SUCCESS ||
				vkCreateSemaphore(contentManager.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) !=
					VK_SUCCESS ||
				vkCreateFence(contentManager.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
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

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clearValues[1].depthStencil = {1.0f, 0};

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = framebuffers[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapChainManager.extent;
		VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->graphicsPipelineManager.pipeline);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapChainManager.extent.width);
		viewport.height = static_cast<float>(swapChainManager.extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapChainManager.extent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkBuffer vertexBuffers[] = {vertexBufferManager.buffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, indexBufferManager.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer,
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								this->graphicsPipelineManager.layout,
								0,
								1,
								&descriptorManager.sets[currentFrame],
								0,
								nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indexBufferManager.indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer!");
		}
	}

	void createFramebuffers()
	{
		this->framebuffers.resize(this->swapChainManager.imageViews.size());

		for (size_t i = 0; i < this->swapChainManager.imageViews.size(); i++)
		{
			std::array<VkImageView, 2> attachments = {swapChainManager.imageViews[i], depthImageManager.imageView};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.pNext = nullptr;
			framebufferInfo.flags = 0;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainManager.extent.width;
			framebufferInfo.height = swapChainManager.extent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(contentManager.device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create framebuffer!");
			}
		}
	}

	void createRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.flags = 0;
		colorAttachment.format = swapChainManager.format;
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

		VkAttachmentDescription depthAttachment{};
		depthAttachment.flags = 0;
		depthAttachment.format = depthImageManager.findDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

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

		std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 2;
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(contentManager.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create render pass!");
		}
	}

	void setupGraphicsPipelines()
	{

		std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		this->graphicsPipelineManager.dynamicStates = dynamicStates;

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.pNext = nullptr;
		vertShaderStageInfo.flags = 0;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertexShaderManager.module;
		vertShaderStageInfo.pName = "main";
		vertShaderStageInfo.pSpecializationInfo = nullptr;

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.pNext = nullptr;
		fragShaderStageInfo.flags = 0;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragmentShaderManager.module;
		fragShaderStageInfo.pName = "main";
		vertShaderStageInfo.pSpecializationInfo = nullptr;

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainManager.extent.width;
		viewport.height = (float)swapChainManager.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapChainManager.extent;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorManager.setLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		graphicsPipelineManager.setRequiredValue(shaderStages, viewport, scissor, pipelineLayoutInfo, renderPass);
	}

	void draw()
	{
		vkWaitForFences(contentManager.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		/* ��ȡ������ͼ�� */
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(contentManager.device,
												this->swapChainManager.swapChain,
												UINT64_MAX,
												imageAvailableSemaphores[currentFrame],
												VK_NULL_HANDLE,
												&imageIndex);

		/* �������봰�ڱ��治���� */
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		vkResetFences(contentManager.device, 1, &inFlightFences[currentFrame]);

		/* ��¼������� */
		vkResetCommandBuffer(commandManager.graphicsCommandBuffers[currentFrame], 0);
		recordCommandBuffer(commandManager.graphicsCommandBuffers[currentFrame], imageIndex);

		updateUniformBuffer(currentFrame);

		/* �ύ������� */
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;

		VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandManager.graphicsCommandBuffers[currentFrame];
		VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(contentManager.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to submit draw command buffer!");
		}

		/* ���г��� */
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = {this->swapChainManager.swapChain};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		result = vkQueuePresentKHR(this->contentManager.presentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
		{
			framebufferResized = false;
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present swap chain image!");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void clear()
	{
		this->uniformBufferManager.clear();

		this->indexBufferManager.clear();

		this->vertexBufferManager.clear();

		this->textureManager.clear();

		for (auto framebuffer : framebuffers)
		{
			vkDestroyFramebuffer(contentManager.device, framebuffer, nullptr);
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(contentManager.device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(contentManager.device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(contentManager.device, inFlightFences[i], nullptr);
		}

		this->graphicsPipelineManager.clear();

		vkDestroyRenderPass(contentManager.device, renderPass, nullptr);

		this->descriptorManager.clear();

		this->depthImageManager.clear();

		this->swapChainManager.clear();

		this->commandManager.clear();

		this->vertexShaderManager.clear();
		this->fragmentShaderManager.clear();

		this->contentManager.clear();
	}

	void updateUniformBuffer(int index)
	{
		this->uniformBufferManager.update(ubo, index);
		ubo.project = glm::perspective(glm::radians(45.0f),
									   (float)swapChainManager.extent.width / (float)swapChainManager.extent.height,
									   0.1f,
									   10.0f);
		ubo.project[1][1] *= -1;
	}

private:
	ContentManager contentManager{};

	SwapChainManager swapChainManager{};

	TextureManager textureManager{};

	CommandManager commandManager{};

	SwapChainManager swapChianManager{};

	VertexBufferManager vertexBufferManager{};

	IndexBufferManager indexBufferManager{};

	UniformBufferManager uniformBufferManager{};

	DepthImageManager depthImageManager{};

	DescriptorManager descriptorManager{};

	ShaderManager vertexShaderManager{};

	ShaderManager fragmentShaderManager{};

	PipelineManager graphicsPipelineManager{};

	/* ��Ⱦͨ�� */
	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffers;

	/* ͬ���� */
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	/* ���ڴ�С������־ */
	bool framebufferResized = false;

	UniformBufferObject ubo{};

	double mousePositionX = 0.0f, mousePositionY = 0.0f;

	uint32_t currentFrame = 0;
};