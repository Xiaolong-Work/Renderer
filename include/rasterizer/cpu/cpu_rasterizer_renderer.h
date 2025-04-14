#pragma once

#include <model.h>
#include <rasterizer.h>
#include <stb_image_write.h>

#include <transform.h>
#include <vulkan_utils.h>

bool keys[1024];
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
			/*float angle = sqrt(differenceX * differenceX + differenceY * differenceY) / 3.6f;
			glm::vec3 rotationAxis(differenceY, differenceX, 0.0f);
			rotationAxis = glm::normalize(rotationAxis);
			self->rasterizer.model = glm::rotate(self->rasterizer.model, glm::radians(angle), rotationAxis);*/
		}

		self->mousePositionX = xpos;
		self->mousePositionY = ypos;
		return;
	}

	CPURasterizerRenderer() = default;

	unsigned int width = 1024, height = 1024;
	Rasterizer rasterizer;
	std::vector<Model> models;

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (key >= 0 && key < 1024)
		{
			if (action == GLFW_PRESS)
			{
				keys[key] = true;
			}
			else if (action == GLFW_RELEASE)
			{
				keys[key] = false;
			}
		}
	}

	glm::vec3 eye = glm::vec3(-24.0f, 24.0f, 21.0f);
	glm::vec3 center = glm::vec3(2.0f, 2.0f, 2.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	void processInput(GLFWwindow* window)
	{
		float cameraSpeed = 0.5f;

		if (keys[GLFW_KEY_W])
		{
			eye += cameraSpeed * glm::normalize(center);
		}
		if (keys[GLFW_KEY_S])
		{
			eye -= cameraSpeed * glm::normalize(center);
		}
		if (keys[GLFW_KEY_A])
		{
			eye -= glm::normalize(glm::cross(center, up)) * cameraSpeed;
		}
		if (keys[GLFW_KEY_D])
		{
			eye += glm::normalize(glm::cross(center, up)) * cameraSpeed;
		}

		rasterizer.view = glm::lookAt(eye, center, up);
	}

	void configuration1()
	{
		Model model{"F:/C++/Renderer/models/viking_room/viking_room.obj",
					"F:/C++/Renderer/models/viking_room/viking_room.png"};
		models.push_back(model);

		rasterizer.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		eye = glm::vec3(2.0f, 2.0f, 2.0f);
		center = glm::vec3(0.0f, 0.0f, 0.0f);
		up = glm::vec3(0.0f, 0.0f, 1.0f);
		rasterizer.view = glm::lookAt(eye, center, up);

		rasterizer.projection = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 10.0f);

		rasterizer.shader = std::function<Vector3f(Shader, const std::vector<std::vector<std::vector<float>>>)>(
			Shader::textureFragmentShader);
	}

	void configuration2()
	{
		Model model{"F:/C++/Renderer/models/light_test/light_test.obj"};
		model.lights.push_back(
			PointLight{Vector3f{3.555117130279541f, 5.7209856510162354f, 7.8868520259857178f}, Vector3f{20.0f, 0.0f, 5.0f}});
		model.lights.push_back(PointLight{Vector3f{10.796123027801514f, 14.8698282837867737f, -10.731630563735962f},
									 Vector3f{500.0f, 60.0f, 9.0f}});
		model.lights.push_back(PointLight{Vector3f{5.061335563659668f, 10.0382213369011879f, 3.3268730640411377f},
									 Vector3f{0.0f, 78.0f, 67.0f}});
		model.lights.push_back(
			PointLight{Vector3f{4.167896270751953f, 7.8664369583129883f, 3.0616421699523926f}, Vector3f{0.0f, 20.0f, 1.0f}});
		models.push_back(model);

		rasterizer.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		eye = glm::vec3(-24.0f, 24.0f, 21.0f);
		center = glm::vec3(2.0f, 2.0f, 2.0f);
		up = glm::vec3(0.0f, 1.0f, 0.0f);

		rasterizer.view = glm::lookAt(eye, center, up);

		rasterizer.projection = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 1000.0f);

		rasterizer.shader = std::function<Vector3f(Shader, const std::vector<std::vector<std::vector<float>>>)>(
			Shader::textureFragmentShader);
	}

	void configuration3()
	{
		Model model{"F:/C++/Renderer/models/bathroom/bathroom.obj"};
		models.push_back(model);

		rasterizer.model = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

		eye = glm::vec3(0.0072405338287353516, 0.9124049544334412, -0.2275838851928711);
		center = glm::vec3(-2.787562608718872, 0.9699121117591858, -2.6775901317596436);
		up = glm::vec3(0.0f, 1.0f, 0.0f);

		rasterizer.view = glm::lookAt(eye, center, up);

		rasterizer.projection = glm::perspective(glm::radians(55.0f), (float)width / height, 0.1f, 10.0f);

		rasterizer.shader = std::function<Vector3f(Shader, const std::vector<std::vector<std::vector<float>>>)>(
			Shader::normalFragmentShader);
	}

	void init()
	{
		DrawFrame::init();

		this->buffer_manager.size = sizeof(float) * 4 * this->width * this->height;
		this->buffer_manager.init();

		rasterizer = Rasterizer(this->width, this->height);

		configuration2();

		rasterizer.genetareShadowMaps(models[0]);

		glfwSetFramebufferSizeCallback(this->content_manager.window, framebufferResizeCallback);
		glfwSetCursorPosCallback(this->content_manager.window, cursorPositionCallback);
		glfwSetWindowUserPointer(this->content_manager.window, this);
		glfwSetKeyCallback(this->content_manager.window, keyCallback);

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

		this->render_pass_manager.swap_chain_manager_sprt.swap(std::make_shared<SwapChainManager>(this->swap_chain_manager));
		this->render_pass_manager.recreate();
	}

	void render() override
	{
		rasterizer.clear();
		for (auto& model : models)
		{
			rasterizer.drawShaderTriangleframe(model);
		}

		// save();

		memcpy(this->buffer_manager.mapped, rasterizer.screen_buffer.data(), sizeof(float) * 4 * width * height);

		this->texture_manager.copyBufferToImage(
			this->buffer_manager.buffer, this->texture_manager.images[0], VkExtent3D{width, height, 1});

		this->texture_manager.transformLayout(this->texture_manager.images[0],
											  VK_FORMAT_R32G32B32A32_SFLOAT,
											  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
											  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		processInput(this->content_manager.window);
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
