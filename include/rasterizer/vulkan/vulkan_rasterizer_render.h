#pragma once

#include <fstream>
#include <iostream>

#include <buffer_manager.h>
#include <command_manager.h>
#include <descriptor_manager.h>
#include <image_manager.h>
#include <pipeline_manager.h>
#include <render_pass_manager.h>
#include <shader_manager.h>
#include <ssbo_buffer_manager.h>
#include <swap_chain_manager.h>
#include <texture_manager.h>

#include <model.h>
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
		while (!glfwWindowShouldClose(context_manager.window))
		{
			auto begin = std::chrono::system_clock::now();
			glfwPollEvents();
			draw();
			auto end = std::chrono::system_clock::now();
			outputFrameRate(end - begin);
		}

		vkDeviceWaitIdle(context_manager.device);
	}

protected:
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto self = reinterpret_cast<VulkanRasterRenderer*>(glfwGetWindowUserPointer(window));
		self->windowResized = true;
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

	void handleWindowResize()
	{
		int width = 0, height = 0;

		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(context_manager.window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(context_manager.device);

		this->swap_chain_manager.recreate();

		this->render_pass_manager.pSwapChainManager.swap(std::make_shared<SwapChainManager>(this->swap_chain_manager));
		this->render_pass_manager.recreate();
	}

	void createUniformBufferObjects()
	{
		ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.project = glm::perspective(glm::radians(45.0f),
									   (float)swap_chain_manager.extent.width / (float)swap_chain_manager.extent.height,
									   0.1f,
									   10.0f);
		ubo.project[1][1] *= -1;

		glfwSetFramebufferSizeCallback(this->context_manager.window, framebufferResizeCallback);
		glfwSetCursorPosCallback(this->context_manager.window, cursorPositionCallback);
		glfwSetWindowUserPointer(this->context_manager.window, this);
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

	void setupGraphicsPipelines()
	{
		std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		this->graphics_pipeline_manager.dynamicStates = dynamicStates;

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
		pipelineLayoutInfo.pSetLayouts = &descriptorManagers[0].layout;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		graphics_pipeline_manager.setRequiredValue(
			shaderStages, viewport, scissor, pipelineLayoutInfo, render_pass_manager.renderPass);
		graphics_pipeline_manager.enableVertexInpute = true;
	}

	void setupDescriptor(const int index)
	{
		/* Layout bingding */
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;
		this->descriptorManagers[index].bindings.push_back(uboLayoutBinding);

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		this->descriptorManagers[index].bindings.push_back(samplerLayoutBinding);

		/* Pool size */
		VkDescriptorPoolSize uboPoolSize{};
		uboPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboPoolSize.descriptorCount = static_cast<uint32_t>(1);
		this->descriptorManagers[index].poolSizes.push_back(uboPoolSize);

		VkDescriptorPoolSize samplerPoolSize{};
		samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerPoolSize.descriptorCount = static_cast<uint32_t>(1);
		this->descriptorManagers[index].poolSizes.push_back(samplerPoolSize);

		this->descriptorManagers[index].init();

		/* Write Descriptor Set  */
		std::vector<VkWriteDescriptorSet> descriptorWrites{};
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = this->uniformBufferManagers[index].uniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);
		VkWriteDescriptorSet bufferWrite;
		bufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		bufferWrite.pNext = nullptr;
		bufferWrite.dstSet = this->descriptorManagers[index].set;
		bufferWrite.dstBinding = 0;
		bufferWrite.dstArrayElement = 0;
		bufferWrite.descriptorCount = 1;
		bufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bufferWrite.pBufferInfo = &bufferInfo;
		bufferWrite.pImageInfo = nullptr;
		bufferWrite.pTexelBufferView = nullptr;
		descriptorWrites.push_back(bufferWrite);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = texture_manager.imageViews[0];
		imageInfo.sampler = texture_manager.sampler;
		VkWriteDescriptorSet imageWrite{};
		imageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		imageWrite.pNext = nullptr;
		imageWrite.dstSet = this->descriptorManagers[index].set;
		imageWrite.dstBinding = 1;
		imageWrite.dstArrayElement = 0;
		imageWrite.descriptorCount = 1;
		imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageWrite.pBufferInfo = nullptr;
		imageWrite.pImageInfo = &imageInfo;
		imageWrite.pTexelBufferView = nullptr;
		descriptorWrites.push_back(imageWrite);

		vkUpdateDescriptorSets(
			context_manager.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	void setupShadowMapRenderPass()
	{
		/* Shadow calculation subpass configurationt */
		VkAttachmentDescription shadow_map_attachment{};
		shadow_map_attachment.flags = 0;
		shadow_map_attachment.format = VK_FORMAT_D32_SFLOAT;
		shadow_map_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		shadow_map_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		shadow_map_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		shadow_map_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		shadow_map_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		shadow_map_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		shadow_map_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		VkAttachmentReference shadow_map_attachment_reference{};
		shadow_map_attachment_reference.attachment = 0;
		shadow_map_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.flags = 0;
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = nullptr;
		subpass.colorAttachmentCount = 0;
		subpass.pColorAttachments = nullptr;
		subpass.pResolveAttachments = nullptr;
		subpass.pDepthStencilAttachment = &shadow_map_attachment_reference;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = nullptr;

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
	}

	void setupRenderPass()
	{
		int attachment_count = 0;
		int subpass_count = 0;
		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkSubpassDescription> subpasses;

		/* Geometry Processing Subpass Configuration */
		std::vector<VkAttachmentDescription> geometry_buffer_attachments{};
		geometry_buffer_attachments.resize(6);

		for (size_t i = 0; i < geometry_buffer_attachments.size(); i++)
		{
			geometry_buffer_attachments[i].flags = 0;
			geometry_buffer_attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
			geometry_buffer_attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			geometry_buffer_attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			geometry_buffer_attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			geometry_buffer_attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			geometry_buffer_attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			geometry_buffer_attachments[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		/* Position attachment */
		geometry_buffer_attachments[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		/* Normal Attachment */
		geometry_buffer_attachments[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		/* Albedo Attachment */
		geometry_buffer_attachments[2].format = VK_FORMAT_R8G8B8A8_UNORM;
		/* Roughness Attachment */
		geometry_buffer_attachments[3].format = VK_FORMAT_R8G8_UNORM;
		/* Material Index Attachment */
		geometry_buffer_attachments[4].format = VK_FORMAT_R8_UINT;
		/* Depth Buffer Attachment */
		geometry_buffer_attachments[5].format = VK_FORMAT_D32_SFLOAT;
		geometry_buffer_attachments[5].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments.insert(attachments.end(), geometry_buffer_attachments.begin(), geometry_buffer_attachments.end());

		std::vector<VkAttachmentReference> geometry_buffer_color_attachment_reference(5);
		for (int i = 0; i < 5; i++)
		{
			geometry_buffer_color_attachment_reference[i].attachment = attachment_count++;
			geometry_buffer_color_attachment_reference[i].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		VkAttachmentReference geometry_buffer_depth_attachment_reference{};
		geometry_buffer_depth_attachment_reference.attachment = attachment_count++;
		geometry_buffer_depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription geometry_buffer_subpass{};
		geometry_buffer_subpass.flags = 0;
		geometry_buffer_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		geometry_buffer_subpass.inputAttachmentCount = static_cast<uint32_t>(0);
		geometry_buffer_subpass.pInputAttachments = nullptr;
		geometry_buffer_subpass.colorAttachmentCount =
			static_cast<uint32_t>(geometry_buffer_color_attachment_reference.size());
		geometry_buffer_subpass.pColorAttachments = geometry_buffer_color_attachment_reference.data();
		geometry_buffer_subpass.pResolveAttachments = nullptr;
		geometry_buffer_subpass.pDepthStencilAttachment = &geometry_buffer_depth_attachment_reference;
		geometry_buffer_subpass.preserveAttachmentCount = static_cast<uint32_t>(0);
		geometry_buffer_subpass.pPreserveAttachments = nullptr;
		subpasses.push_back(geometry_buffer_subpass);

		/* Framebuffer image color attachment */
		VkAttachmentDescription frame_color_attachment{};
		frame_color_attachment.flags = 0;
		frame_color_attachment.format = swap_chain_manager.format;
		frame_color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		frame_color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		frame_color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		frame_color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		frame_color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		frame_color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		frame_color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachments.push_back(frame_color_attachment);

		VkAttachmentReference frame_color_attachment_reference{};
		frame_color_attachment_reference.attachment = attachment_count++;
		frame_color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription light_compute_subpass{};
		light_compute_subpass.flags = 0;
		light_compute_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		light_compute_subpass.inputAttachmentCount = 0;	   // TODO
		light_compute_subpass.pInputAttachments = nullptr; // TODO
		light_compute_subpass.colorAttachmentCount = 1;
		light_compute_subpass.pColorAttachments = &frame_color_attachment_reference;
		light_compute_subpass.pResolveAttachments = nullptr;
		light_compute_subpass.pDepthStencilAttachment = nullptr;
		light_compute_subpass.preserveAttachmentCount = 0;
		light_compute_subpass.pPreserveAttachments = nullptr;
		subpasses.push_back(light_compute_subpass);

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &frame_color_attachment_reference;
		subpass.pDepthStencilAttachment = nullptr;

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

		// std::array<VkAttachmentDescription, 2> attachments = {frame_color_attachment, depthAttachment};
		// VkRenderPassCreateInfo renderPassInfo{};
		// renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		// renderPassInfo.attachmentCount = 2;
		// renderPassInfo.pAttachments = attachments.data();
		// renderPassInfo.subpassCount = 1;
		// renderPassInfo.pSubpasses = &subpass;
		// renderPassInfo.dependencyCount = 1;
		// renderPassInfo.pDependencies = &dependency;

		// if (vkCreateRenderPass(contentManager.device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		//{
		//	throw std::runtime_error("Failed to create render pass!");
		// }
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

	void init()
	{
		// temp
		this->lights.push_back(Light{});

		this->context_manager.init();
		auto pContentManager = std::make_shared<ContextManager>(this->context_manager);

		this->command_manager = CommandManager(pContentManager);
		this->command_manager.init();
		auto pCommandManager = std::make_shared<CommandManager>(this->command_manager);

		this->swap_chain_manager = SwapChainManager(pContentManager, pCommandManager);
		this->swap_chain_manager.init();
		auto pSwapChainManager = std::make_shared<SwapChainManager>(this->swap_chain_manager);

		this->point_light_shadow_map_image_manager = PointLightShadowMapImageManager(pContentManager, pCommandManager);
		this->point_light_shadow_map_image_manager.light_number = this->lights.size();
		this->point_light_shadow_map_image_manager.init();

		this->render_pass_manager = RenderPassManager(pContentManager, pSwapChainManager, pCommandManager);
		this->render_pass_manager.init();

		this->vertex_shader_manager = ShaderManager(pContentManager);
		this->vertex_shader_manager.setShaderName("rasterize_vert.spv");
		this->vertex_shader_manager.init();

		this->fragment_shader_manager = ShaderManager(pContentManager);
		this->fragment_shader_manager.setShaderName("rasterize_frag.spv");
		this->fragment_shader_manager.init();

		this->texture_manager = TextureManager(pContentManager, pCommandManager);
		this->texture_manager.init();
		this->texture_manager.createTexture(TEXTURE_PATH);

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			this->uniformBufferManagers[i] = UniformBufferManager(pContentManager, pCommandManager);
			this->uniformBufferManagers[i].init();

			this->descriptorManagers[i] = DescriptorManager(pContentManager);
			this->setupDescriptor(i);
		}

		this->vertexBufferManager = VertexBufferManager(pContentManager, pCommandManager);
		this->indexBufferManager = IndexBufferManager(pContentManager, pCommandManager);
		loadModel();
		this->vertexBufferManager.init();
		this->indexBufferManager.init();

		this->graphics_pipeline_manager = PipelineManager(pContentManager);
		this->setupGraphicsPipelines();
		this->graphics_pipeline_manager.init();

		createSyncObjects();

		createUniformBufferObjects();
	}

	void clear()
	{
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(context_manager.device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(context_manager.device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(context_manager.device, inFlightFences[i], nullptr);

			this->uniformBufferManagers[i].clear();
			this->descriptorManagers[i].clear();
		}

		this->indexBufferManager.clear();

		this->vertexBufferManager.clear();

		this->texture_manager.clear();

		this->graphics_pipeline_manager.clear();

		this->render_pass_manager.clear();

		this->point_light_shadow_map_image_manager.clear();

		this->swap_chain_manager.clear();

		this->command_manager.clear();

		this->vertex_shader_manager.clear();

		this->fragment_shader_manager.clear();

		this->context_manager.clear();
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
		renderPassInfo.renderPass = render_pass_manager.renderPass;
		renderPassInfo.framebuffer = render_pass_manager.framebuffers[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swap_chain_manager.extent;
		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clearValues[1].depthStencil = {1.0f, 0};
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->graphics_pipeline_manager.pipeline);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swap_chain_manager.extent.width);
		viewport.height = static_cast<float>(swap_chain_manager.extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swap_chain_manager.extent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkBuffer vertexBuffers[] = {vertexBufferManager.buffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, indexBufferManager.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer,
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								this->graphics_pipeline_manager.layout,
								0,
								1,
								&descriptorManagers[currentFrame].set,
								0,
								nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indexBufferManager.indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to record command buffer!");
		}
	}

	void updateUniformBufferObjects(int index)
	{
		this->uniformBufferManagers[index].update(ubo);
		ubo.project = glm::perspective(glm::radians(45.0f),
									   (float)swap_chain_manager.extent.width / (float)swap_chain_manager.extent.height,
									   0.1f,
									   10.0f);
		ubo.project[1][1] *= -1;
	}

	void draw()
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
			handleWindowResize();
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

		updateUniformBufferObjects(currentFrame);

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

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || windowResized)
		{
			windowResized = false;
			handleWindowResize();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present swap chain image!");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

private:
	ContextManager context_manager{};

	SwapChainManager swap_chain_manager{};

	TextureManager texture_manager{};

	CommandManager command_manager{};

	SwapChainManager swapChianManager{};

	VertexBufferManager vertexBufferManager{};

	IndexBufferManager indexBufferManager{};

	std::array<UniformBufferManager, MAX_FRAMES_IN_FLIGHT> uniformBufferManagers{};

	std::array<DescriptorManager, MAX_FRAMES_IN_FLIGHT> descriptorManagers{};

	ShaderManager vertex_shader_manager{};

	ShaderManager fragment_shader_manager{};

	PipelineManager graphics_pipeline_manager{};

	RenderPassManager render_pass_manager{};

	UniformBufferObject ubo{};

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	uint32_t currentFrame = 0;

	bool windowResized = false;
	double mousePositionX = 0.0f, mousePositionY = 0.0f;

	std::vector<Light> lights;

	PointLightShadowMapImageManager point_light_shadow_map_image_manager{};
};