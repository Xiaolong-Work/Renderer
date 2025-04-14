#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>

#include <buffer_manager.h>
#include <command_manager.h>
#include <descriptor_manager.h>
#include <image_manager.h>
#include <pipeline_manager.h>
#include <render_pass_manager.h>
#include <shader_manager.h>
#include <shadow_map_manager.h>
#include <ssbo_buffer_manager.h>
#include <swap_chain_manager.h>
#include <texture_manager.h>

#include <scene.h>

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
		int count = 0;
		float sum = 0.0f;
		while (!glfwWindowShouldClose(context_manager.window))
		{
			auto begin = std::chrono::system_clock::now();
			glfwPollEvents();
			draw();
			auto end = std::chrono::system_clock::now();

			if (sum >= 1000000.0f)
			{
				outputFrameRate(count);
				count = 0;
				sum = 0.0f;
			}
			else
			{
				count++;
				sum += (end - begin).count();
			}
		}

		vkDeviceWaitIdle(context_manager.device);
	}

	void setData(const Scene& scene)
	{
		/* Processing vertex data */
		for (size_t i = 0; i < scene.objects.size(); i++)
		{
			auto& object = scene.objects[i];

			VkDrawIndexedIndirectCommand indirect{};
			indirect.indexCount = static_cast<uint32_t>(object.indices.size());
			indirect.instanceCount = 1;
			indirect.firstIndex = static_cast<uint32_t>(this->index_buffer_manager.indices.size());
			indirect.vertexOffset = static_cast<int32_t>(this->vertex_buffer_manager.vertices.size());
			indirect.firstInstance = 0;

			this->vertex_buffer_manager.vertices.insert(
				this->vertex_buffer_manager.vertices.end(), object.vertices.begin(), object.vertices.end());

			this->index_buffer_manager.indices.insert(
				this->index_buffer_manager.indices.end(), object.indices.begin(), object.indices.end());

			this->indirect_buffer_manager.commands.push_back(indirect);
		}

		this->vertex_buffer_manager.init();
		this->index_buffer_manager.init();
		this->indirect_buffer_manager.init();

		/* Processing Material Data */
		std::vector<Index> material_index;
		for (auto& object : scene.objects)
		{
			material_index.push_back(object.material_index);
		}
		this->material_index_manager.setData(
			material_index.data(), sizeof(Index) * material_index.size(), material_index.size());
		this->material_index_manager.init();

		std::vector<SSBOMaterial> materials;
		for (auto& material : scene.materials)
		{
			SSBOMaterial ssbo_material{material};
			materials.push_back(ssbo_material);
		}
		this->material_ssbo_manager.setData(
			materials.data(), sizeof(SSBOMaterial) * materials.size(), materials.size());
		this->material_ssbo_manager.init();

		/* Processing texture data */
		for (auto& texture : scene.textures)
		{
			this->texture_manager.createTexture(texture.getPath());
		}

		this->shadow_map_manager =
			ShadowMapManager(std::make_shared<ContextManager>(this->context_manager),
							 std::make_shared<CommandManager>(this->command_manager),
							 std::make_shared<VertexBufferManager>(this->vertex_buffer_manager),
							 std::make_shared<IndexBufferManager>(this->index_buffer_manager),
							 std::make_shared<IndirectBufferManager>(this->indirect_buffer_manager));

		this->shadow_map_manager.point_lights = scene.point_lights;

		// temp
		this->shadow_map_manager.point_lights.push_back(PointLight{{7, 2, 1}, {50.0f, 50.0f, 50.0f}});

		this->shadow_map_manager.init();

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			this->uniformBufferManagers[i].init();
			this->setupDescriptor(i);
		}

		this->setupGraphicsPipelines();
		this->graphics_pipeline_manager.init();
	}

protected:
	glm::vec3 cameraPosition = glm::vec3(15, 5, 1);
	
	glm::vec3 cameraFront = glm::vec3(1, 0, 0);
	glm::vec3 cameraUp = glm::vec3(0, 1, 0);

	float yaw = -90.0f; // 初始朝 -Z 方向
	float pitch = 0.0f;
	bool firstMouse = true;

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto self = reinterpret_cast<VulkanRasterRenderer*>(glfwGetWindowUserPointer(window));
		self->windowResized = true;
	}

	bool VulkanRasterRenderer::mouseCaptured = false;

	static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
	{
		auto self = static_cast<VulkanRasterRenderer*>(glfwGetWindowUserPointer(window));

		if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
		{
			if (self->mouseCaptured)
			{
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // 显示鼠标
				self->mouseCaptured = false;
				self->firstMouse = true;
			}
			return;
		}

		if (!self->mouseCaptured && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // 隐藏并锁定鼠标
			self->mouseCaptured = true;
			self->firstMouse = true;
			return;
		}

		if (!self->mouseCaptured)
		{
			return;
		}

		// FPS 鼠标控制逻辑
		if (self->firstMouse)
		{
			self->mousePositionX = xpos;
			self->mousePositionY = ypos;
			self->firstMouse = false;
		}

		double dx = xpos - self->mousePositionX;
		double dy = self->mousePositionY - ypos;

		self->mousePositionX = xpos;
		self->mousePositionY = ypos;

		const float sensitivity = 0.1f;
		self->yaw += (float)dx * sensitivity;
		self->pitch += (float)dy * sensitivity;

		self->pitch = clamp(-89.0f, 89.0f, self->pitch);

		glm::vec3 direction{};
		direction.x = cos(glm::radians(self->yaw)) * cos(glm::radians(self->pitch));
		direction.y = sin(glm::radians(self->pitch));
		direction.z = sin(glm::radians(self->yaw)) * cos(glm::radians(self->pitch));
		self->cameraFront = glm::normalize(direction);

		self->ubo.view = glm::lookAt(self->cameraPosition, self->cameraPosition + self->cameraFront, self->cameraUp);
	}

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{

		auto self = static_cast<VulkanRasterRenderer*>(glfwGetWindowUserPointer(window));

		const float moveSpeed = 0.05f;
		glm::vec3 forward = glm::vec3(self->ubo.view[0][2], self->ubo.view[1][2], self->ubo.view[2][2]);
		glm::vec3 right = glm::vec3(self->ubo.view[0][0], self->ubo.view[1][0], self->ubo.view[2][0]);
		glm::vec3 up = glm::vec3(self->ubo.view[0][1], self->ubo.view[1][1], self->ubo.view[2][1]);

		switch (key)
		{
		case GLFW_KEY_W:
			{
				self->cameraPosition -= forward * moveSpeed;
				break;
			}
		case GLFW_KEY_A:
			{
				self->cameraPosition -= right * moveSpeed;
				break;
			}
		case GLFW_KEY_S:
			{
				self->cameraPosition += forward * moveSpeed;
				break;
			}
		case GLFW_KEY_D:
			{
				self->cameraPosition += right * moveSpeed;
				break;
			}
		case GLFW_KEY_SPACE:
			{
				self->cameraPosition += up * moveSpeed;
				break;
			}
		case GLFW_KEY_LEFT_SHIFT:
			{
				self->cameraPosition -= up * moveSpeed;
				break;
			}
		default:
			break;
		}

		self->ubo.view = glm::lookAt(self->cameraPosition, self->cameraPosition - forward, up);
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

		this->render_pass_manager.swap_chain_manager_sprt.swap(
			std::make_shared<SwapChainManager>(this->swap_chain_manager));
		this->render_pass_manager.recreate();
	}

	void createUniformBufferObjects()
	{
		ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);
		ubo.project = glm::perspective(glm::radians(90.0f),
									   (float)swap_chain_manager.extent.width / (float)swap_chain_manager.extent.height,
									   0.1f,
									   1000.0f);
		ubo.project[1][1] *= -1;

		glfwSetFramebufferSizeCallback(this->context_manager.window, framebufferResizeCallback);
		glfwSetCursorPosCallback(this->context_manager.window, cursorPositionCallback);
		glfwSetKeyCallback(this->context_manager.window, keyCallback);
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
		this->graphics_pipeline_manager.dynamic_states = dynamicStates;

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
		pipelineLayoutInfo.pSetLayouts = &descriptor_managers[0].layout;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		graphics_pipeline_manager.setRequiredValue(
			shaderStages, viewport, scissor, pipelineLayoutInfo, render_pass_manager.pass);
		graphics_pipeline_manager.enable_vertex_inpute = true;
	}

	void setupDescriptor(const int index)
	{
		/* ========== Layout binding infomation ========== */
		/* MVP UBO binding */
		VkDescriptorSetLayoutBinding ubo_layout_binding{};
		ubo_layout_binding.binding = 0;
		ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_layout_binding.descriptorCount = 1;
		ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		ubo_layout_binding.pImmutableSamplers = nullptr;
		this->descriptor_managers[index].bindings.push_back(ubo_layout_binding);

		/* Texture sampler binding */
		VkDescriptorSetLayoutBinding sampler_layout_binding{};
		sampler_layout_binding.binding = 1;
		sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_layout_binding.descriptorCount = static_cast<uint32_t>(this->texture_manager.imageViews.size());
		sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		sampler_layout_binding.pImmutableSamplers = nullptr;
		this->descriptor_managers[index].bindings.push_back(sampler_layout_binding);

		/* Point light shadow map sampler binding */
		VkDescriptorSetLayoutBinding point_light_shadow_map_layout_binding{};
		point_light_shadow_map_layout_binding.binding = 2;
		point_light_shadow_map_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		point_light_shadow_map_layout_binding.descriptorCount = 1;
		point_light_shadow_map_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		point_light_shadow_map_layout_binding.pImmutableSamplers = nullptr;
		this->descriptor_managers[index].bindings.push_back(point_light_shadow_map_layout_binding);

		/* Point light data binding */
		VkDescriptorSetLayoutBinding point_light_layout_binding{};
		point_light_layout_binding.binding = 3;
		point_light_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		point_light_layout_binding.descriptorCount =
			static_cast<uint32_t>(this->shadow_map_manager.point_light_manager.length);
		point_light_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		point_light_layout_binding.pImmutableSamplers = nullptr;
		this->descriptor_managers[index].bindings.push_back(point_light_layout_binding);

		/* Material index bingding */
		VkDescriptorSetLayoutBinding material_index_layout_binding{};
		material_index_layout_binding.binding = 4;
		material_index_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		material_index_layout_binding.descriptorCount = static_cast<uint32_t>(this->material_index_manager.length);
		material_index_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		material_index_layout_binding.pImmutableSamplers = nullptr;
		this->descriptor_managers[index].bindings.push_back(material_index_layout_binding);

		/* Material data binding */
		VkDescriptorSetLayoutBinding material_data_layout_binding{};
		material_data_layout_binding.binding = 5;
		material_data_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		material_data_layout_binding.descriptorCount = static_cast<uint32_t>(this->material_ssbo_manager.length);
		material_data_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		material_data_layout_binding.pImmutableSamplers = nullptr;
		this->descriptor_managers[index].bindings.push_back(material_data_layout_binding);

		/* Point light mvp binding */
		VkDescriptorSetLayoutBinding point_light_mvp_layout_binding{};
		point_light_mvp_layout_binding.binding = 6;
		point_light_mvp_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		point_light_mvp_layout_binding.descriptorCount =
			static_cast<uint32_t>(this->shadow_map_manager.mvp_ssbo_manager.length);
		point_light_mvp_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		point_light_mvp_layout_binding.pImmutableSamplers = nullptr;
		this->descriptor_managers[index].bindings.push_back(point_light_mvp_layout_binding);

		/* ========== Pool size infomation ========== */
		/* UBO pool size */
		VkDescriptorPoolSize ubo_pool_size{};
		ubo_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_pool_size.descriptorCount = 1;
		this->descriptor_managers[index].poolSizes.push_back(ubo_pool_size);

		/* Sampler pool size */
		VkDescriptorPoolSize sampler_pool_size{};
		sampler_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_pool_size.descriptorCount = 2;
		this->descriptor_managers[index].poolSizes.push_back(sampler_pool_size);

		/* SSBO pool size */
		VkDescriptorPoolSize ssbo_pool_size{};
		ssbo_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		ssbo_pool_size.descriptorCount = 4;
		this->descriptor_managers[index].poolSizes.push_back(ssbo_pool_size);

		this->descriptor_managers[index].init();

		/* ========== Write Descriptor Set ========== */
		std::vector<VkWriteDescriptorSet> descriptor_writes{};

		/* MVP UBO set write information */
		VkDescriptorBufferInfo mvp_buffer_information{};
		mvp_buffer_information.buffer = this->uniformBufferManagers[index].buffer;
		mvp_buffer_information.offset = 0;
		mvp_buffer_information.range = sizeof(UniformBufferObject);
		VkWriteDescriptorSet mvp_buffer_write;
		mvp_buffer_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		mvp_buffer_write.pNext = nullptr;
		mvp_buffer_write.dstSet = this->descriptor_managers[index].set;
		mvp_buffer_write.dstBinding = 0;
		mvp_buffer_write.dstArrayElement = 0;
		mvp_buffer_write.descriptorCount = 1;
		mvp_buffer_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		mvp_buffer_write.pBufferInfo = &mvp_buffer_information;
		mvp_buffer_write.pImageInfo = nullptr;
		mvp_buffer_write.pTexelBufferView = nullptr;
		descriptor_writes.push_back(mvp_buffer_write);

		/* Texture sampler write information */
		if (!this->texture_manager.imageViews.empty())
		{
			std::vector<VkDescriptorImageInfo> texture_information;
			texture_information.resize(this->texture_manager.imageViews.size());
			for (size_t i = 0; i < this->texture_manager.imageViews.size(); i++)
			{
				texture_information[i].imageView = texture_manager.imageViews[i];
				texture_information[i].sampler = texture_manager.sampler;
				texture_information[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}
			VkWriteDescriptorSet texture_write{};
			texture_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			texture_write.pNext = nullptr;
			texture_write.dstSet = this->descriptor_managers[index].set;
			texture_write.dstBinding = 1;
			texture_write.dstArrayElement = 0;
			texture_write.descriptorCount = static_cast<uint32_t>(texture_information.size());
			texture_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			texture_write.pBufferInfo = nullptr;
			texture_write.pImageInfo = texture_information.data();
			texture_write.pTexelBufferView = nullptr;
			descriptor_writes.push_back(texture_write);
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
			shadow_write.dstBinding = 2;
			shadow_write.dstArrayElement = 0;
			shadow_write.descriptorCount = 1;
			shadow_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			shadow_write.pBufferInfo = nullptr;
			shadow_write.pImageInfo = &shadow_information;
			shadow_write.pTexelBufferView = nullptr;
			descriptor_writes.push_back(shadow_write);
		}

		/* Point light data write information */
		if (!this->shadow_map_manager.point_lights.empty())
		{
			VkDescriptorBufferInfo point_light_data_buffer_information;
			point_light_data_buffer_information.buffer = this->shadow_map_manager.point_light_manager.buffer;
			point_light_data_buffer_information.offset = 0;
			point_light_data_buffer_information.range = VK_WHOLE_SIZE;
			VkWriteDescriptorSet point_light_data_buffer_write;
			point_light_data_buffer_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			point_light_data_buffer_write.pNext = nullptr;
			point_light_data_buffer_write.dstSet = this->descriptor_managers[index].set;
			point_light_data_buffer_write.dstBinding = 3;
			point_light_data_buffer_write.dstArrayElement = 0;
			point_light_data_buffer_write.descriptorCount = 1;
			point_light_data_buffer_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			point_light_data_buffer_write.pBufferInfo = &point_light_data_buffer_information;
			point_light_data_buffer_write.pImageInfo = nullptr;
			point_light_data_buffer_write.pTexelBufferView = nullptr;
			descriptor_writes.push_back(point_light_data_buffer_write);
		}

		/* Material index set write information */
		VkDescriptorBufferInfo material_index_buffer_information;
		material_index_buffer_information.buffer = this->material_index_manager.buffer;
		material_index_buffer_information.offset = 0;
		material_index_buffer_information.range = VK_WHOLE_SIZE;
		VkWriteDescriptorSet material_index_buffer_write;
		material_index_buffer_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		material_index_buffer_write.pNext = nullptr;
		material_index_buffer_write.dstSet = this->descriptor_managers[index].set;
		material_index_buffer_write.dstBinding = 4;
		material_index_buffer_write.dstArrayElement = 0;
		material_index_buffer_write.descriptorCount = 1;
		material_index_buffer_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		material_index_buffer_write.pBufferInfo = &material_index_buffer_information;
		material_index_buffer_write.pImageInfo = nullptr;
		material_index_buffer_write.pTexelBufferView = nullptr;
		descriptor_writes.push_back(material_index_buffer_write);

		/* Material data set write information */
		VkDescriptorBufferInfo material_data_buffer_information;
		material_data_buffer_information.buffer = this->material_ssbo_manager.buffer;
		material_data_buffer_information.offset = 0;
		material_data_buffer_information.range = VK_WHOLE_SIZE;
		VkWriteDescriptorSet material_data_buffer_write;
		material_data_buffer_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		material_data_buffer_write.pNext = nullptr;
		material_data_buffer_write.dstSet = this->descriptor_managers[index].set;
		material_data_buffer_write.dstBinding = 5;
		material_data_buffer_write.dstArrayElement = 0;
		material_data_buffer_write.descriptorCount = 1;
		material_data_buffer_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		material_data_buffer_write.pBufferInfo = &material_data_buffer_information;
		material_data_buffer_write.pImageInfo = nullptr;
		material_data_buffer_write.pTexelBufferView = nullptr;
		descriptor_writes.push_back(material_data_buffer_write);

		VkDescriptorBufferInfo point_light_mvp_buffer_information;
		point_light_mvp_buffer_information.buffer = this->shadow_map_manager.mvp_ssbo_manager.buffer;
		point_light_mvp_buffer_information.offset = 0;
		point_light_mvp_buffer_information.range = VK_WHOLE_SIZE;
		VkWriteDescriptorSet point_light_mvp_buffer_write;
		point_light_mvp_buffer_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		point_light_mvp_buffer_write.pNext = nullptr;
		point_light_mvp_buffer_write.dstSet = this->descriptor_managers[index].set;
		point_light_mvp_buffer_write.dstBinding = 6;
		point_light_mvp_buffer_write.dstArrayElement = 0;
		point_light_mvp_buffer_write.descriptorCount = 1;
		point_light_mvp_buffer_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		point_light_mvp_buffer_write.pBufferInfo = &point_light_mvp_buffer_information;
		point_light_mvp_buffer_write.pImageInfo = nullptr;
		point_light_mvp_buffer_write.pTexelBufferView = nullptr;
		descriptor_writes.push_back(point_light_mvp_buffer_write);

		vkUpdateDescriptorSets(context_manager.device,
							   static_cast<uint32_t>(descriptor_writes.size()),
							   descriptor_writes.data(),
							   0,
							   nullptr);
	}

	void setupShadowMapRenderPass()
	{
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

		// if (vkCreateRenderPass(contentManager.device, &renderPassInfo, nullptr, &pass) != VK_SUCCESS)
		//{
		//	throw std::runtime_error("Failed to create render pass!");
		// }
	}

	void init()
	{
		// temp
		this->lights.push_back(PointLight{});

		this->context_manager.init();
		auto context_manager_sptr = std::make_shared<ContextManager>(this->context_manager);

		this->command_manager = CommandManager(context_manager_sptr);
		this->command_manager.init();
		auto command_manager_sptr = std::make_shared<CommandManager>(this->command_manager);

		this->swap_chain_manager = SwapChainManager(context_manager_sptr, command_manager_sptr);
		this->swap_chain_manager.init();
		auto swap_chain_manager_sptr = std::make_shared<SwapChainManager>(this->swap_chain_manager);

		this->render_pass_manager =
			RenderPassManager(context_manager_sptr, swap_chain_manager_sptr, command_manager_sptr);
		this->render_pass_manager.init();

		this->vertex_shader_manager = ShaderManager(context_manager_sptr);
		this->vertex_shader_manager.setShaderName("rasterize_vert.spv");
		this->vertex_shader_manager.init();

		this->fragment_shader_manager = ShaderManager(context_manager_sptr);
		this->fragment_shader_manager.setShaderName("rasterize_frag.spv");
		this->fragment_shader_manager.init();

		this->texture_manager = TextureManager(context_manager_sptr, command_manager_sptr);
		this->texture_manager.init();

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			this->uniformBufferManagers[i] = UniformBufferManager(context_manager_sptr, command_manager_sptr);
			this->descriptor_managers[i] = DescriptorManager(context_manager_sptr);
		}

		this->vertex_buffer_manager = VertexBufferManager(context_manager_sptr, command_manager_sptr);

		this->index_buffer_manager = IndexBufferManager(context_manager_sptr, command_manager_sptr);

		this->indirect_buffer_manager = IndirectBufferManager(context_manager_sptr, command_manager_sptr);

		this->material_ssbo_manager = StorageBufferManager(context_manager_sptr, command_manager_sptr);
		this->material_index_manager = StorageBufferManager(context_manager_sptr, command_manager_sptr);

		this->graphics_pipeline_manager = PipelineManager(context_manager_sptr);

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
			this->descriptor_managers[i].clear();
		}

		this->material_index_manager.clear();

		this->material_ssbo_manager.clear();

		this->indirect_buffer_manager.clear();

		this->index_buffer_manager.clear();

		this->vertex_buffer_manager.clear();

		this->texture_manager.clear();

		this->graphics_pipeline_manager.clear();

		this->render_pass_manager.clear();

		this->shadow_map_manager.clear();

		this->swap_chain_manager.clear();

		this->command_manager.clear();

		this->vertex_shader_manager.clear();

		this->fragment_shader_manager.clear();

		this->context_manager.clear();
	}

	void recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(command_buffer, &beginInfo) != VK_SUCCESS)
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

		vkCmdBeginRenderPass(command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->graphics_pipeline_manager.pipeline);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swap_chain_manager.extent.width);
		viewport.height = static_cast<float>(swap_chain_manager.extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(command_buffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swap_chain_manager.extent;
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);

		VkBuffer vertex_buffers[] = {vertex_buffer_manager.buffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

		vkCmdBindIndexBuffer(command_buffer, index_buffer_manager.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(command_buffer,
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								this->graphics_pipeline_manager.layout,
								0,
								1,
								&descriptor_managers[currentFrame].set,
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

	void updateUniformBufferObjects(int index)
	{
		ubo.project = glm::perspective(glm::radians(90.0f),
									   (float)swap_chain_manager.extent.width / (float)swap_chain_manager.extent.height,
									   0.1f,
									   1000.0f);
		ubo.project[1][1] *= -1;
		this->uniformBufferManagers[index].update(ubo);
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

	VertexBufferManager vertex_buffer_manager{};

	IndexBufferManager index_buffer_manager{};

	IndirectBufferManager indirect_buffer_manager{};

	StorageBufferManager material_ssbo_manager{};

	StorageBufferManager material_index_manager{};

	ShadowMapManager shadow_map_manager{};

	std::array<UniformBufferManager, MAX_FRAMES_IN_FLIGHT> uniformBufferManagers{};

	std::array<DescriptorManager, MAX_FRAMES_IN_FLIGHT> descriptor_managers{};

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

	std::vector<PointLight> lights;
};