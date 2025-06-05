#pragma once

#include <descriptor_manager.h>
#include <image_manager.h>
#include <pipeline_manager.h>
#include <shader_manager.h>
#include <texture_manager.h>

#include <light.h>
#include <render_pass_manager.h>
#include <stb_image_write.h>

class DirectionLightShadowMapManager : public ImageManager
{
public:
	DirectionLightShadowMapManager() = default;
	DirectionLightShadowMapManager(const ContextManagerSPtr& context_manager_sptr,
								   const CommandManagerSPtr& command_manager_sptr)
	{
		this->context_manager_sptr = context_manager_sptr;
		this->command_manager_sptr = command_manager_sptr;
	}

	void init()
	{
		createImageResource();
		createSampler();
	}
	void clear()
	{
		vkDestroyImage(context_manager_sptr->device, this->image, nullptr);
		vkFreeMemory(context_manager_sptr->device, this->memory, nullptr);
		vkDestroyImageView(context_manager_sptr->device, this->view, nullptr);
		vkDestroyImage(context_manager_sptr->device, this->depth_image, nullptr);
		vkFreeMemory(context_manager_sptr->device, this->depth_memory, nullptr);
		vkDestroyImageView(context_manager_sptr->device, this->depth_view, nullptr);
		vkDestroySampler(context_manager_sptr->device, this->sampler, nullptr);
	}

	VkImage image{};
	VkDeviceMemory memory{};
	VkImageView view{};

	VkImage depth_image{};
	VkDeviceMemory depth_memory{};
	VkImageView depth_view{};

	VkSampler sampler{};

	int light_number{0};
	unsigned int width{1024};
	unsigned int height{1024};

protected:
	void createImageResource()
	{
		VkImageCreateInfo image_information{};
		image_information.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_information.pNext = nullptr;
		image_information.flags = 0;
		image_information.imageType = VK_IMAGE_TYPE_2D;
		image_information.format = VK_FORMAT_R32_SFLOAT;
		image_information.extent = VkExtent3D{this->width, this->height, 1};
		image_information.mipLevels = 1;
		image_information.arrayLayers = static_cast<uint32_t>(this->light_number);
		image_information.samples = VK_SAMPLE_COUNT_1_BIT;
		image_information.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_information.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		image_information.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_information.queueFamilyIndexCount = 0;
		image_information.pQueueFamilyIndices = nullptr;
		image_information.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		if (vkCreateImage(context_manager_sptr->device, &image_information, nullptr, &this->image) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image!");
		}

		image_information.format = VK_FORMAT_D32_SFLOAT;
		image_information.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		if (vkCreateImage(context_manager_sptr->device, &image_information, nullptr, &this->depth_image) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image!");
		}

		auto bindMemory = [this](VkImage& image, VkDeviceMemory& memory) {
			VkMemoryRequirements memory_requirements;
			vkGetImageMemoryRequirements(context_manager_sptr->device, image, &memory_requirements);

			VkMemoryAllocateInfo allocate_information{};
			allocate_information.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocate_information.allocationSize = memory_requirements.size;
			allocate_information.memoryTypeIndex =
				findMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			if (vkAllocateMemory(context_manager_sptr->device, &allocate_information, nullptr, &memory) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to allocate image memory!");
			}

			vkBindImageMemory(context_manager_sptr->device, image, memory, 0);
		};
		bindMemory(this->image, this->memory);
		bindMemory(this->depth_image, this->depth_memory);

		VkImageViewCreateInfo view_information{};
		view_information.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_information.pNext = nullptr;
		view_information.flags = 0;
		view_information.image = this->image;
		view_information.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		view_information.format = VK_FORMAT_R32_SFLOAT;
		view_information.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_information.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_information.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_information.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		view_information.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view_information.subresourceRange.baseMipLevel = 0;
		view_information.subresourceRange.levelCount = 1;
		view_information.subresourceRange.baseArrayLayer = 0;
		view_information.subresourceRange.layerCount = static_cast<uint32_t>(this->light_number);
		if (vkCreateImageView(context_manager_sptr->device, &view_information, nullptr, &this->view) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image view!");
		}

		view_information.image = this->depth_image;
		view_information.format = VK_FORMAT_D32_SFLOAT;
		view_information.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (vkCreateImageView(context_manager_sptr->device, &view_information, nullptr, &this->depth_view) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image view!");
		}
	}

	void createSampler()
	{
		VkSamplerCreateInfo sampler_information{};
		sampler_information.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler_information.pNext = nullptr;
		sampler_information.flags = 0;
		sampler_information.magFilter = VK_FILTER_NEAREST;
		sampler_information.minFilter = VK_FILTER_NEAREST;
		sampler_information.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		sampler_information.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_information.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_information.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		sampler_information.mipLodBias = 0.0f;
		sampler_information.anisotropyEnable = VK_FALSE;
		sampler_information.maxAnisotropy = 1.0f;
		sampler_information.compareEnable = VK_FALSE;
		sampler_information.compareOp = VK_COMPARE_OP_ALWAYS;
		sampler_information.minLod = 0.0f;
		sampler_information.maxLod = 0.0f;
		sampler_information.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		sampler_information.unnormalizedCoordinates = VK_FALSE;

		if (vkCreateSampler(context_manager_sptr->device, &sampler_information, nullptr, &this->sampler) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create texture sampler!");
		}
	}
};

class ShadowMapManager
{
public:
	ShadowMapManager() = default;
	ShadowMapManager(const ContextManagerSPtr& context_manager_sptr,
					 const CommandManagerSPtr& command_manager_sptr,
					 const VertexBufferManagerSPtr& vertex_buffer_manager_sptr,
					 const IndexBufferManagerSPtr& index_buffer_manager_sptr,
					 const IndirectBufferManagerSPtr& indirect_buffer_manager_sptr,
					 const StorageBufferManagerSPtr& model_matrix_manager_sptr,
					 const TextureManagerSPtr& texture_manager_sptr,
					 const StorageBufferManagerSPtr material_index_manager_sptr,
					 const StorageBufferManagerSPtr material_ssbo_manager);

	void init();

	void clear();

	void createFrameBuffer()
	{
		std::array<VkImageView, 2> views{this->point_light_shadow_map_manager.view,
										 this->point_light_shadow_map_manager.depth_view};
		VkFramebufferCreateInfo frame_buffer_create_information{};
		frame_buffer_create_information.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frame_buffer_create_information.pNext = nullptr;
		frame_buffer_create_information.flags = 0;
		frame_buffer_create_information.renderPass = this->render_pass_manager.pass;
		frame_buffer_create_information.attachmentCount = static_cast<uint32_t>(views.size());
		frame_buffer_create_information.pAttachments = views.data();
		frame_buffer_create_information.width = this->width;
		frame_buffer_create_information.height = this->height;
		frame_buffer_create_information.layers = static_cast<uint32_t>(6 * this->point_lights.size());

		if (vkCreateFramebuffer(
				context_manager_sptr->device, &frame_buffer_create_information, nullptr, &this->frame_buffers) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create framebuffer!");
		}
	}

	void setupIndirectBuffer()
	{
		this->indirect_buffer_manager.commands = this->indirect_buffer_manager_sptr->commands;
		for (auto& draw_command : this->indirect_buffer_manager.commands)
		{
			draw_command.instanceCount = static_cast<uint32_t>(this->point_lights.size() * 6);
		}
		this->indirect_buffer_manager.init();
	}

	void setupMVPSSBO()
	{
		struct LightMVP
		{
			alignas(16) glm::mat4 model;
			alignas(16) glm::mat4 view[6];
			alignas(16) glm::mat4 project;
			alignas(16) glm::vec3 position;
		};

		std::vector<Direction> ups = {Direction(0, -1, 0),
									  Direction(0, -1, 0),
									  Direction(0, 0, 1),
									  Direction(0, 0, -1),
									  Direction(0, -1, 0),
									  Direction(0, -1, 0)};

		std::vector<Vector3f> looks{Direction(1, 0, 0),
									Direction(-1, 0, 0),
									Direction(0, 1, 0),
									Direction(0, -1, 0),
									Direction(0, 0, 1),
									Direction(0, 0, -1)};

		std::vector<LightMVP> mvps{};
		/* For each point light */
		for (size_t i = 0; i < this->point_lights.size(); i++)
		{
			/* For each face */
			LightMVP mvp{};
			mvp.model = glm::mat4{1.0f};
			mvp.project = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
			mvp.position = this->point_lights[i].position;

			for (size_t j = 0; j < 6; j++)
			{
				auto& light_position = this->point_lights[i].position;
				mvp.view[j] = glm::lookAt(light_position, light_position + looks[j], ups[j]);
			}
			mvps.push_back(mvp);
		}
		/* For direction point light */
		for (size_t i = 0; i < this->direction_lights.size(); i++)
		{
			LightMVP mvp{};
			// Temp
			float scale = 20;

			auto& light_position = this->direction_lights[i].position;
			auto& light_direction = this->direction_lights[i].direction;

			mvp.model = glm::mat4{1.0f};
			mvp.project = glm::ortho(-scale, scale, -scale, scale, -scale, scale);
			mvp.view[0] = glm::lookAt(light_position, light_position + light_direction, Direction(0, 1, 0));
			mvp.position = light_position;
		}

		this->mvp_ssbo_manager.setData(mvps.data(), mvps.size() * sizeof(LightMVP), mvps.size());
		this->mvp_ssbo_manager.init();
	}

	void setupDescriptor()
	{
		/* ========== Layout bingding infomation ========== */
		/* Point light MVP SSBO binding */
		this->descriptor_manager.addLayoutBinding(
			this->mvp_ssbo_manager.getLayoutBinding(0, VK_SHADER_STAGE_VERTEX_BIT));

		/* Model matrix SSBO binding */
		this->descriptor_manager.addLayoutBinding(
			this->model_matrix_manager_sptr->getLayoutBinding(1, VK_SHADER_STAGE_VERTEX_BIT));

		/* Texture Image binding */
		this->descriptor_manager.addLayoutBinding(
			this->texture_manager_sptr->getLayoutBinding(2, VK_SHADER_STAGE_FRAGMENT_BIT));

		/* Material index bingding */
		this->descriptor_manager.addLayoutBinding(
			this->material_index_manager_sptr->getLayoutBinding(3, VK_SHADER_STAGE_FRAGMENT_BIT));

		/* Material data binding */
		this->descriptor_manager.addLayoutBinding(
			this->material_ssbo_manager_sptr->getLayoutBinding(4, VK_SHADER_STAGE_FRAGMENT_BIT));

		/* ========== Write Descriptor Set ========== */
		/* Point light MVP SSBO set write information */
		std::vector<VkWriteDescriptorSet> writes{};
		this->descriptor_manager.addWrite(this->mvp_ssbo_manager.getWriteInformation(0));
		this->descriptor_manager.addWrite(this->model_matrix_manager_sptr->getWriteInformation(1));
		this->descriptor_manager.addWrite(this->texture_manager_sptr->getWriteInformation(2));
		this->descriptor_manager.addWrite(this->material_index_manager_sptr->getWriteInformation(3));
		this->descriptor_manager.addWrite(this->material_ssbo_manager_sptr->getWriteInformation(4));

		this->descriptor_manager.init();
	}

	void setupGraphicsPipelines()
	{
		this->point_pipeline_manager.addShaderStage("shadow_point_generate_vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		this->point_pipeline_manager.addShaderStage("shadow_generate_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		this->point_pipeline_manager.setDefaultFixedState();
		this->point_pipeline_manager.setExtent(VkExtent2D{this->width, this->height});
		this->point_pipeline_manager.setRenderPass(this->render_pass_manager.pass);
		this->point_pipeline_manager.setVertexInput(0b1010);
		std::vector<VkDescriptorSetLayout> layout = {this->descriptor_manager.layout};
		this->point_pipeline_manager.setLayout(layout);
		this->point_pipeline_manager.init();
	}

	void recordCommandBuffer(VkCommandBuffer command_buffer)
	{
		VkRenderPassBeginInfo render_pass_begin{};
		render_pass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_begin.renderPass = render_pass_manager.pass;
		render_pass_begin.framebuffer = this->frame_buffers;
		render_pass_begin.renderArea.offset = {0, 0};
		render_pass_begin.renderArea.extent = {this->point_light_shadow_map_manager.width,
											   this->point_light_shadow_map_manager.height};
		std::array<VkClearValue, 2> clear_values{};
		clear_values[0].depthStencil = {1.0f, 0};
		clear_values[1].color = {1.0f};
		render_pass_begin.clearValueCount = 2;
		render_pass_begin.pClearValues = clear_values.data();

		vkCmdBeginRenderPass(command_buffer, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->point_pipeline_manager.pipeline);

		VkBuffer vertex_buffers[] = {this->vertex_buffer_manager_sptr->buffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

		vkCmdBindIndexBuffer(command_buffer, this->index_buffer_manager_sptr->buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(command_buffer,
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								this->point_pipeline_manager.layout,
								0,
								1,
								&this->descriptor_manager.set,
								0,
								nullptr);

		vkCmdDrawIndexedIndirect(command_buffer,
								 this->indirect_buffer_manager.buffer,
								 0,
								 static_cast<uint32_t>(this->indirect_buffer_manager.commands.size()),
								 sizeof(VkDrawIndexedIndirectCommand));

		vkCmdEndRenderPass(command_buffer);
	}

	void renderShadowMap()
	{
		auto command_buffer = command_manager_sptr->beginGraphicsCommands();
		recordCommandBuffer(command_buffer);
		command_manager_sptr->endGraphicsCommands(command_buffer);
	}

	std::vector<PointLight> point_lights{};

	std::vector<DirectionLight> direction_lights{};

	PointLightShadowMapImageManager point_light_shadow_map_manager{};

	DirectionLightShadowMapManager direction_light_shadow_map_manager{};

	StorageBufferManager point_light_manager{};

	StorageBufferManager mvp_ssbo_manager{};

protected:
private:
	ContextManagerSPtr context_manager_sptr;
	CommandManagerSPtr command_manager_sptr;

	VertexBufferManagerSPtr vertex_buffer_manager_sptr;
	IndexBufferManagerSPtr index_buffer_manager_sptr;
	IndirectBufferManagerSPtr indirect_buffer_manager_sptr;

	StorageBufferManagerSPtr model_matrix_manager_sptr;
	TextureManagerSPtr texture_manager_sptr;
	StorageBufferManagerSPtr material_index_manager_sptr;
	StorageBufferManagerSPtr material_ssbo_manager_sptr;

	IndirectBufferManager indirect_buffer_manager{};

	DescriptorManager descriptor_manager{};

	RenderPassManager render_pass_manager{};

	PipelineManager point_pipeline_manager{};
	PipelineManager direction_pipeline_manager{};

	VkFramebuffer frame_buffers;

	unsigned int width{1024};
	unsigned int height{1024};
};
