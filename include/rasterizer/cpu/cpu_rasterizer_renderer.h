#pragma once

#include <model.h>
#include <rasterizer.h>
#include <stb_image_write.h>

#include <vulkan_utils.h>

class CPURasterizerRenderer : public DrawFrame
{
public:
	bool windowResized = false;
	double mousePositionX = 0.0f, mousePositionY = 0.0f;

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto self = reinterpret_cast<CPURasterizerRenderer*>(glfwGetWindowUserPointer(window));
		self->windowResized = true;
	}

	static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
	{
		auto self = static_cast<CPURasterizerRenderer*>(glfwGetWindowUserPointer(window));

		double differenceX = xpos - self->mousePositionX;
		double differenceY = ypos - self->mousePositionY;

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{
			float angle = sqrt(differenceX * differenceX + differenceY * differenceY) / 3.6f;
			glm::vec3 rotationAxis(differenceY, differenceX, 0.0f);
			rotationAxis = glm::normalize(rotationAxis);
			self->rasterizer.model = glm::rotate(self->rasterizer.model, glm::radians(angle), rotationAxis);
		}

		self->mousePositionX = xpos;
		self->mousePositionY = ypos;
		return;
	}

	CPURasterizerRenderer() = default;

	unsigned int width = 1024, height = 1024;
	Rasterizer rasterizer;
	std::vector<Model> models;

	void init()
	{
		DrawFrame::init();

		this->buffer_manager.size = sizeof(float) * 3 * this->width * this->height;
		this->buffer_manager.init();

		Model model{"F:/C++/Renderer/models/viking_room/viking_room.obj",
					"F:/C++/Renderer/models/viking_room/viking_room.png"};
		models.push_back(model);

		rasterizer = Rasterizer(this->width, this->height);
		rasterizer.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		rasterizer.view =
			glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		rasterizer.projection = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 10.0f);

		rasterizer.shader = std::function<Vector3f(Shader)>(Shader::textureFragmentShader);

		glfwSetFramebufferSizeCallback(this->content_manager.window, framebufferResizeCallback);
		glfwSetCursorPosCallback(this->content_manager.window, cursorPositionCallback);
		glfwSetWindowUserPointer(this->content_manager.window, this);

		handleWindowResize();
	}

	void handleWindowResize()
	{
		int width = 0, height = 0;

		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(content_manager.window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(content_manager.device);

		size_t size = sizeof(float) * 4 * this->width * this->height;

		this->buffer_manager.clear();
		this->buffer_manager.size = size;
		this->buffer_manager.init();

		this->texture_manager.clear();
		this->texture_manager.init();
		this->texture_manager.setExtent(VkExtent2D{unsigned(width), unsigned(height)});
		this->texture_manager.createEmptyTexture();

		/* Write Descriptor Set  */
		VkDescriptorImageInfo image_infomation{};
		image_infomation.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_infomation.imageView = this->texture_manager.imageViews[0];
		image_infomation.sampler = this->texture_manager.sampler;
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

		

		this->swap_chain_manager.recreate();

		this->render_pass_manager.pSwapChainManager.swap(std::make_shared<SwapChainManager>(this->swap_chain_manager));
		this->render_pass_manager.recreate();
	}

	void render() override
	{
		rasterizer.clear();
		for (auto& model : models)
		{
			rasterizer.drawShaderTriangleframe(model);
		}

		memcpy(this->buffer_manager.mapped, rasterizer.screen_buffer.data(), sizeof(float) * 4 * width * height);

		this->texture_manager.copyBufferToImage(
			this->buffer_manager.buffer, this->texture_manager.images[0], VkExtent3D{width, height, 1});

		this->texture_manager.transformLayout(this->texture_manager.images[0],
											  VK_FORMAT_R32G32B32A32_SFLOAT,
											  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
											  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void clear()
	{
		DrawFrame::clear();
	}

	void save()
	{
		std::string path = std::string(ROOT_DIR) + "/results/" + "1_cpu.bmp";

		std::vector<unsigned char> image_data(width * height * 3);
		for (auto i = 0; i < height * width; ++i)
		{
			static unsigned char color[3];
			image_data[i * 3 + 0] = (unsigned char)(255 * std::pow(clamp(0, 1, rasterizer.screen_buffer[i].x), 1.0f));
			image_data[i * 3 + 1] = (unsigned char)(255 * std::pow(clamp(0, 1, rasterizer.screen_buffer[i].y), 1.0f));
			image_data[i * 3 + 2] = (unsigned char)(255 * std::pow(clamp(0, 1, rasterizer.screen_buffer[i].z), 1.0f));
		}

		stbi_write_bmp(path.c_str(), width, height, 3, image_data.data());
	}
};
