#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>

#include <buffer_manager.h>
#include <command_manager.h>
#include <descriptor_manager.h>
#include <geometry_buffer_manager.h>
#include <image_manager.h>
#include <pipeline_manager.h>
#include <render_pass_manager.h>
#include <shader_manager.h>
#include <shadow_map_manager.h>
#include <ssbo_buffer_manager.h>
#include <swap_chain_manager.h>
#include <texture_manager.h>

#include <scene.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#define MAX_FRAMES_IN_FLIGHT 2

class VulkanRendererBase
{
public:
	VulkanRendererBase() = default;
	~VulkanRendererBase() = default;

	int frames_per_second{0};
	double frame_time{0};

	void run()
	{
		int frame_count = 0;
		auto last = std::chrono::high_resolution_clock::now();

		while (!glfwWindowShouldClose(this->context_manager.window))
		{
			auto now = std::chrono::high_resolution_clock::now();
			auto elapsed = std::chrono::duration<double, std::milli>(now - last).count();

			glfwPollEvents();
			draw();

			auto next = std::chrono::high_resolution_clock::now();
			this->frame_time = std::chrono::duration<double, std::milli>(next - now).count();

			outputFrameRate(this->frames_per_second, this->frame_time);

			frame_count++;
			if (elapsed >= 1000.0f)
			{
				this->frames_per_second = frame_count;
				frame_count = 0;
				last = now;
			}
		}

		vkDeviceWaitIdle(this->context_manager.device);
	}

	void setData(const Scene& scene)
	{
		/* Processing vertex data */
		for (size_t i = 0; i < scene.objects.size(); i++)
		{
			auto& object = scene.objects[i];

			VkDrawIndexedIndirectCommand command{};
			command.indexCount = static_cast<uint32_t>(object.indices.size());
			command.instanceCount = 1;
			command.firstIndex = static_cast<uint32_t>(this->index_buffer_manager.indices.size());
			command.vertexOffset = static_cast<int32_t>(this->vertex_buffer_manager.vertices.size());
			command.firstInstance = 0;

			this->vertex_buffer_manager.vertices.insert(
				this->vertex_buffer_manager.vertices.end(), object.vertices.begin(), object.vertices.end());
			this->vertex_buffer_manager.vertex_count.push_back(object.vertices.size());

			this->index_buffer_manager.indices.insert(
				this->index_buffer_manager.indices.end(), object.indices.begin(), object.indices.end());

			this->indirect_buffer_manager.commands.push_back(command);
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
		if (!materials.empty())
		{
			this->material_ssbo_manager.setData(
				materials.data(), sizeof(SSBOMaterial) * materials.size(), materials.size());
			this->material_ssbo_manager.init();
		}

		std::vector<Matrix4f> model_matrixes{};
		for (auto& object : scene.objects)
		{
			model_matrixes.push_back(object.model);
		}
		this->model_matrix_manager.setData(
			model_matrixes.data(), sizeof(Matrix4f) * model_matrixes.size(), model_matrixes.size());
		this->model_matrix_manager.init();

		/* Processing texture data */
		for (auto& texture : scene.textures)
		{
			this->texture_manager.createTexture(texture);
			this->texture_manager.createSampler(texture);
		}
		if (scene.textures.empty())
		{
			this->texture_manager.createEmptyTexture();
		}

		this->shadow_map_manager =
			ShadowMapManager(std::make_shared<ContextManager>(this->context_manager),
							 std::make_shared<CommandManager>(this->command_manager),
							 std::make_shared<VertexBufferManager>(this->vertex_buffer_manager),
							 std::make_shared<IndexBufferManager>(this->index_buffer_manager),
							 std::make_shared<IndirectBufferManager>(this->indirect_buffer_manager),
							 std::make_shared<StorageBufferManager>(this->model_matrix_manager),
							 std::make_shared<TextureManager>(this->texture_manager),
							 std::make_shared<StorageBufferManager>(this->material_index_manager),
							 std::make_shared<StorageBufferManager>(this->material_ssbo_manager));

		if (!scene.point_lights.empty())
		{
			this->shadow_map_manager.point_lights = scene.point_lights;
			this->shadow_map_manager.init();
		}

		this->camera_position = scene.camera.position;
		this->camera_up = scene.camera.up;
		this->camera_look = scene.camera.look;
		this->fovy = scene.camera.fov;

		Direction direction = glm::normalize(scene.camera.look - scene.camera.position);
		yaw = glm::degrees(atan2(direction.z, direction.x));
		pitch = glm::degrees(asin(direction.y));

		createUniformBufferObjects();
	}

	void setWindowSize(const Scene& scene)
	{
		this->width = scene.camera.width;
		this->heigth = scene.camera.height;
	}

protected:
	Coordinate3D camera_position{};
	Direction camera_look{};
	Direction camera_up{};
	float fovy{90.0f};

	uint32_t width{1024};
	uint32_t heigth{1024};

	float yaw{0.0f};
	float pitch{0.0f};

	bool firstMouse = true;
	bool mouseCaptured = false;

	bool window_resized = false;
	double mousePositionX = 0.0f, mousePositionY = 0.0f;
	bool moved{false};

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto self = reinterpret_cast<VulkanRendererBase*>(glfwGetWindowUserPointer(window));
		self->window_resized = true;
	}

	static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
	{
		auto self = static_cast<VulkanRendererBase*>(glfwGetWindowUserPointer(window));

		if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
		{
			if (self->mouseCaptured)
			{
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // ��ʾ���
				self->mouseCaptured = false;
				self->firstMouse = true;
			}
			return;
		}

		if (!self->mouseCaptured && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // ���ز��������
			self->mouseCaptured = true;
			self->firstMouse = true;
			return;
		}

		if (!self->mouseCaptured)
		{
			return;
		}

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
		{
			std::cout << self->camera_position.x << "," << self->camera_position.y << "," << self->camera_position.z
					  << std::endl;
		}

		// FPS �������߼�
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
		self->camera_look = glm::normalize(direction) + self->camera_position;

		self->ubo.view = glm::lookAt(self->camera_position, self->camera_look, self->camera_up);

		self->moved = true;
	}

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		auto self = static_cast<VulkanRendererBase*>(glfwGetWindowUserPointer(window));

		const float moveSpeed = 0.5f;
		glm::vec3 forward = self->camera_look - self->camera_position;
		glm::vec3 up = self->camera_up;
		glm::vec3 right = glm::cross(forward, up);

		self->moved = true;
		switch (key)
		{
		case GLFW_KEY_W:
			{
				self->camera_position += forward * moveSpeed;
				break;
			}
		case GLFW_KEY_A:
			{
				self->camera_position -= right * moveSpeed;
				break;
			}
		case GLFW_KEY_S:
			{
				self->camera_position -= forward * moveSpeed;
				break;
			}
		case GLFW_KEY_D:
			{
				self->camera_position += right * moveSpeed;
				break;
			}
		case GLFW_KEY_SPACE:
			{
				self->camera_position += up * moveSpeed;
				break;
			}
		case GLFW_KEY_LEFT_SHIFT:
			{
				self->camera_position -= up * moveSpeed;
				break;
			}
		default:
			{
				self->moved = false;
				break;
			}
		}

		self->camera_look = self->camera_position + forward;
		self->ubo.view = glm::lookAt(self->camera_position, self->camera_look, up);
	}

	void setupImgui()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();

		ImGui_ImplGlfw_InitForVulkan(this->context_manager.window, true);

		ImGui_ImplVulkan_InitInfo init = {};
		init.Instance = this->context_manager.instance;
		init.PhysicalDevice = this->context_manager.physical_device;
		init.Device = this->context_manager.device;
		init.Queue = this->context_manager.graphics_queue;
		init.DescriptorPoolSize = 2;
		init.MinImageCount = this->swap_chain_manager.images.size();
		init.ImageCount = this->swap_chain_manager.images.size();
		init.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init.QueueFamily = this->context_manager.graphics_family;
		init.PipelineCache = VK_NULL_HANDLE;
		init.Allocator = nullptr;
		init.CheckVkResultFn = nullptr;
		init.RenderPass = this->render_pass_manager.pass;
		init.Subpass = this->render_pass_manager.subpasses.size() - 1;

		ImGui_ImplVulkan_Init(&init);
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

		this->ubo.project =
			glm::perspective(glm::radians(this->fovy),
							 (float)swap_chain_manager.extent.width / (float)swap_chain_manager.extent.height,
							 0.1f,
							 1000.0f);
		this->ubo.project[1][1] *= -1;
	}

	void createUniformBufferObjects()
	{
		ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(this->camera_position, this->camera_look, this->camera_up);
		ubo.project = glm::perspective(glm::radians(this->fovy),
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
		this->image_available.resize(MAX_FRAMES_IN_FLIGHT);
		this->render_finished.resize(MAX_FRAMES_IN_FLIGHT);
		this->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

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
			if (vkCreateSemaphore(context_manager.device, &semaphoreInfo, nullptr, &image_available[i]) != VK_SUCCESS ||
				vkCreateSemaphore(context_manager.device, &semaphoreInfo, nullptr, &render_finished[i]) != VK_SUCCESS ||
				vkCreateFence(context_manager.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			{

				throw std::runtime_error("Failed to create synchronization objects for a frame!");
			}
		}
	}

	void init(const uint32_t mask = 0)
	{
		if (mask == 1)
		{
			this->context_manager.enable_ray_tracing = true;
		}

		this->context_manager.setExtent(VkExtent2D{this->width, this->heigth});
		this->context_manager.init();
		auto context_manager_sptr = std::make_shared<ContextManager>(this->context_manager);

		this->command_manager = CommandManager(context_manager_sptr);
		this->command_manager.init();
		auto command_manager_sptr = std::make_shared<CommandManager>(this->command_manager);

		this->swap_chain_manager = SwapChainManager(context_manager_sptr, command_manager_sptr);
		this->swap_chain_manager.init();
		auto swap_chain_manager_sptr = std::make_shared<SwapChainManager>(this->swap_chain_manager);

		this->vertex_buffer_manager = VertexBufferManager(context_manager_sptr, command_manager_sptr);
		this->index_buffer_manager = IndexBufferManager(context_manager_sptr, command_manager_sptr);
		this->indirect_buffer_manager = IndirectBufferManager(context_manager_sptr, command_manager_sptr);
		this->model_matrix_manager = StorageBufferManager(context_manager_sptr, command_manager_sptr);

		this->material_ssbo_manager = StorageBufferManager(context_manager_sptr, command_manager_sptr);
		this->material_index_manager = StorageBufferManager(context_manager_sptr, command_manager_sptr);

		this->texture_manager = TextureManager(context_manager_sptr, command_manager_sptr);
		this->texture_manager.init();

		createSyncObjects();

		if (mask == 1)
		{
			this->index_buffer_manager.enable_ray_tracing = true;
		}
	}

	void clear()
	{

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(context_manager.device, render_finished[i], nullptr);
			vkDestroySemaphore(context_manager.device, image_available[i], nullptr);
			vkDestroyFence(context_manager.device, inFlightFences[i], nullptr);
		}

		this->render_pass_manager.clear();

		this->shadow_map_manager.clear();

		this->model_matrix_manager.clear();

		this->material_index_manager.clear();

		this->material_ssbo_manager.clear();

		this->indirect_buffer_manager.clear();

		this->index_buffer_manager.clear();

		this->vertex_buffer_manager.clear();

		this->command_manager.clear();

		this->texture_manager.clear();

		this->swap_chain_manager.clear();

		this->context_manager.clear();
	}

	virtual void recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index) = 0;

	virtual void updateUniformBufferObjects(int index) = 0;

	void draw()
	{
		/* Wait vkQueueSubmit finish */
		vkWaitForFences(context_manager.device, 1, &inFlightFences[current_frame], VK_TRUE, UINT64_MAX);

		/* Get swap chain image */
		uint32_t image_index;
		VkResult result = vkAcquireNextImageKHR(context_manager.device,
												this->swap_chain_manager.swap_chain,
												UINT64_MAX,
												image_available[current_frame],
												VK_NULL_HANDLE,
												&image_index);

		/* The swap chain is incompatible with the window surface */
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			handleWindowResize();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire swap chain image!");
		}

		vkResetFences(context_manager.device, 1, &inFlightFences[current_frame]);

		/* Recording Command Buffers */
		vkResetCommandBuffer(command_manager.graphicsCommandBuffers[current_frame], 0);
		updateUniformBufferObjects(current_frame);
		recordCommandBuffer(command_manager.graphicsCommandBuffers[current_frame], image_index);

		/* Submitting a command buffer */
		VkPipelineStageFlags flag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submit_information{};
		submit_information.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_information.pNext = nullptr;
		submit_information.waitSemaphoreCount = 1;
		submit_information.pWaitSemaphores = &image_available[current_frame];
		submit_information.pWaitDstStageMask = &flag;
		submit_information.commandBufferCount = 1;
		submit_information.pCommandBuffers = &command_manager.graphicsCommandBuffers[current_frame];
		submit_information.signalSemaphoreCount = 1;
		submit_information.pSignalSemaphores = &render_finished[current_frame];
		result = vkQueueSubmit(context_manager.graphics_queue, 1, &submit_information, inFlightFences[current_frame]);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to submit draw command buffer!");
		}

		/* Presentation */
		VkPresentInfoKHR present_information{};
		present_information.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_information.pNext = nullptr;
		present_information.waitSemaphoreCount = 1;
		present_information.pWaitSemaphores = &render_finished[current_frame];
		present_information.swapchainCount = 1;
		present_information.pSwapchains = &this->swap_chain_manager.swap_chain;
		present_information.pImageIndices = &image_index;
		present_information.pResults = nullptr;

		result = vkQueuePresentKHR(this->context_manager.present_queue, &present_information);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window_resized)
		{
			handleWindowResize();
			window_resized = false;
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to present swap chain image!");
		}

		current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	ContextManager context_manager{};

	SwapChainManager swap_chain_manager{};

	TextureManager texture_manager{};

	CommandManager command_manager{};

	VertexBufferManager vertex_buffer_manager{};

	IndexBufferManager index_buffer_manager{};

	IndirectBufferManager indirect_buffer_manager{};

	StorageBufferManager material_ssbo_manager{};

	StorageBufferManager material_index_manager{};

	StorageBufferManager model_matrix_manager{};

	ShadowMapManager shadow_map_manager{};

	RenderPassManager render_pass_manager{};

	UBOMVP ubo{};

	std::vector<VkSemaphore> image_available;
	std::vector<VkSemaphore> render_finished;
	std::vector<VkFence> inFlightFences;

	uint32_t current_frame = 0;
};