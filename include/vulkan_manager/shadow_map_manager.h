#pragma once

#include <descriptor_manager.h>
#include <image_manager.h>
#include <pipeline_manager.h>
#include <shader_manager.h>

#include <light.h>
#include <render_pass_manager.h>
#include <stb_image_write.h>

class ShadowMapManager
{
public:
	ShadowMapManager() = default;
	ShadowMapManager(const ContextManagerSPtr& context_manager_sptr,
					 const CommandManagerSPtr& command_manager_sptr,
					 const VertexBufferManagerSPtr& vertex_buffer_manager_sptr,
					 const IndexBufferManagerSPtr& index_buffer_manager_sptr,
					 const IndirectBufferManagerSPtr& indirect_buffer_manager_sptr)
	{
		this->context_manager_sptr = context_manager_sptr;
		this->command_manager_sptr = command_manager_sptr;
		this->vertex_buffer_manager_sptr = vertex_buffer_manager_sptr;
		this->index_buffer_manager_sptr = index_buffer_manager_sptr;
		this->indirect_buffer_manager_sptr = indirect_buffer_manager_sptr;
	}

	void init()
	{
		this->vertex_shader_manager = ShaderManager(this->context_manager_sptr);
		this->vertex_shader_manager.setShaderName("shadow_generate_vert.spv");
		this->vertex_shader_manager.init();

		this->fragment_shader_manager = ShaderManager(this->context_manager_sptr);
		this->fragment_shader_manager.setShaderName("shadow_generate_frag.spv");
		this->fragment_shader_manager.init();

		this->indirect_buffer_manager = IndirectBufferManager(this->context_manager_sptr, this->command_manager_sptr);
		this->setupIndirectBuffer();

		this->mvp_ssbo_manager = StorageBufferManager(context_manager_sptr, command_manager_sptr);
		this->setupMVPSSBO();

		this->descriptor_manager = DescriptorManager(context_manager_sptr);
		this->setupDescriptor();

		this->point_light_shadow_map_manager =
			PointLightShadowMapImageManager(this->context_manager_sptr, this->command_manager_sptr);
		this->point_light_shadow_map_manager.light_number = this->point_lights.size();
		this->point_light_shadow_map_manager.init();

		this->render_pass_manager = RenderPassManager(this->context_manager_sptr, this->command_manager_sptr);
		this->render_pass_manager.init();
		this->createFrameBuffer();

		this->graphics_pipeline_manager = PipelineManager(context_manager_sptr);
		this->setupGraphicsPipelines();

		this->point_light_manager = StorageBufferManager(context_manager_sptr, command_manager_sptr);
		this->point_light_manager.setData(
			this->point_lights.data(), sizeof(PointLight) * this->point_lights.size(), this->point_lights.size());
		this->point_light_manager.init();

		this->renderShadowMap();
	}

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
		frame_buffer_create_information.layers = 1;

		if (vkCreateFramebuffer(
				context_manager_sptr->device, &frame_buffer_create_information, nullptr, &this->frame_buffers) !=
			VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create framebuffer!");
		}
	}

	void clear()
	{
		this->point_light_manager.clear();

		this->point_light_shadow_map_manager.clear();

		this->graphics_pipeline_manager.clear();

		this->descriptor_manager.clear();

		this->mvp_ssbo_manager.clear();

		this->indirect_buffer_manager.clear();

		this->fragment_shader_manager.clear();

		this->vertex_shader_manager.clear();
	}

	void setupIndirectBuffer()
	{
		this->indirect_buffer_manager.commands = this->indirect_buffer_manager_sptr->commands;
		for (auto& draw_command : this->indirect_buffer_manager.commands)
		{
			draw_command.instanceCount = static_cast<uint32_t>(this->point_lights.size());
		}
		this->indirect_buffer_manager.init();
	}

	void setupMVPSSBO()
	{
		struct PointLightMVP
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

		std::vector<PointLightMVP> mvps;
		/* For each point light */
		for (size_t i = 0; i < this->point_lights.size(); i++)
		{
			/* For each face */
			PointLightMVP mvp{};
			mvp.model = glm::mat4{1.0f};
			mvp.project = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 20.0f);
			mvp.project[1][1] *= -1;
			mvp.position = this->point_lights[i].position;

			for (size_t j = 0; j < 6; j++)
			{
				auto& light_position = this->point_lights[i].position;
				mvp.view[j] = glm::lookAt(light_position, light_position + looks[j], ups[j]);
			}
			mvps.push_back(mvp);
		}

		this->mvp_ssbo_manager.setData(mvps.data(), mvps.size() * sizeof(PointLightMVP), mvps.size());
		this->mvp_ssbo_manager.init();
	}

	void setupDescriptor()
	{
		/* ========== Layout bingding infomation ========== */
		/* Point light MVP SSBO bingding */
		VkDescriptorSetLayoutBinding mvp_ssbo_layout_binding{};
		mvp_ssbo_layout_binding.binding = 0;
		mvp_ssbo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		mvp_ssbo_layout_binding.descriptorCount = static_cast<uint32_t>(this->point_lights.size());
		mvp_ssbo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		mvp_ssbo_layout_binding.pImmutableSamplers = nullptr;
		this->descriptor_manager.bindings.push_back(mvp_ssbo_layout_binding);

		/* ========== Pool size infomation ========== */
		/* Point light MVP SSBO pool size */
		VkDescriptorPoolSize mvp_ssbo_pool_size{};
		mvp_ssbo_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		mvp_ssbo_pool_size.descriptorCount = 1;
		this->descriptor_manager.poolSizes.push_back(mvp_ssbo_pool_size);

		this->descriptor_manager.init();

		/* ========== Write Descriptor Set ========== */
		/* Point light MVP SSBO set write information */
		VkDescriptorBufferInfo mvp_buffer_information{};
		mvp_buffer_information.buffer = this->mvp_ssbo_manager.buffer;
		mvp_buffer_information.offset = 0;
		mvp_buffer_information.range = VK_WHOLE_SIZE;
		VkWriteDescriptorSet mvp_buffer_write;
		mvp_buffer_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		mvp_buffer_write.pNext = nullptr;
		mvp_buffer_write.dstSet = this->descriptor_manager.set;
		mvp_buffer_write.dstBinding = 0;
		mvp_buffer_write.dstArrayElement = 0;
		mvp_buffer_write.descriptorCount = 1;
		mvp_buffer_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		mvp_buffer_write.pBufferInfo = &mvp_buffer_information;
		mvp_buffer_write.pImageInfo = nullptr;
		mvp_buffer_write.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(context_manager_sptr->device, 1, &mvp_buffer_write, 0, nullptr);
	}

	void setupGraphicsPipelines()
	{
		VkPipelineShaderStageCreateInfo vertex_shader_stage{};
		vertex_shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertex_shader_stage.pNext = nullptr;
		vertex_shader_stage.flags = 0;
		vertex_shader_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertex_shader_stage.module = vertex_shader_manager.module;
		vertex_shader_stage.pName = "main";
		vertex_shader_stage.pSpecializationInfo = nullptr;

		VkPipelineShaderStageCreateInfo fragement_shader_stage{};
		fragement_shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragement_shader_stage.pNext = nullptr;
		fragement_shader_stage.flags = 0;
		fragement_shader_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragement_shader_stage.module = fragment_shader_manager.module;
		fragement_shader_stage.pName = "main";
		vertex_shader_stage.pSpecializationInfo = nullptr;

		std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {vertex_shader_stage, fragement_shader_stage};

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(this->width);
		viewport.height = static_cast<float>(this->height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = VkExtent2D{this->width, this->height};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptor_manager.layout;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		this->graphics_pipeline_manager.setRequiredValue(
			shader_stages, viewport, scissor, pipelineLayoutInfo, this->render_pass_manager.pass);
		this->graphics_pipeline_manager.enable_vertex_inpute = true;

		this->graphics_pipeline_manager.init();
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
		render_pass_begin.clearValueCount = 2u;
		render_pass_begin.pClearValues = clear_values.data();

		vkCmdBeginRenderPass(command_buffer, &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->graphics_pipeline_manager.pipeline);

		VkBuffer vertex_buffers[] = {this->vertex_buffer_manager_sptr->buffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

		vkCmdBindIndexBuffer(command_buffer, this->index_buffer_manager_sptr->buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(command_buffer,
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								this->graphics_pipeline_manager.layout,
								0u,
								1u,
								&this->descriptor_manager.set,
								0u,
								nullptr);

		vkCmdDrawIndexedIndirect(command_buffer,
								 this->indirect_buffer_manager.buffer,
								 0u,
								 static_cast<uint32_t>(this->indirect_buffer_manager_sptr->commands.size()),
								 sizeof(VkDrawIndexedIndirectCommand));

		vkCmdEndRenderPass(command_buffer);
	}

	void renderShadowMap()
	{

		auto command_buffer = command_manager_sptr->beginGraphicsCommands();
		recordCommandBuffer(command_buffer);
		command_manager_sptr->endGraphicsCommands(command_buffer);

		this->point_light_shadow_map_manager.transformLayout(this->point_light_shadow_map_manager.image,
															 VK_FORMAT_R32_SFLOAT,
															 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
															 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
															 6);
	}

	std::vector<PointLight> point_lights;

	PointLightShadowMapImageManager point_light_shadow_map_manager{};

	StorageBufferManager point_light_manager{};

	StorageBufferManager mvp_ssbo_manager{};

protected:
private:
	ContextManagerSPtr context_manager_sptr;
	CommandManagerSPtr command_manager_sptr;

	VertexBufferManagerSPtr vertex_buffer_manager_sptr;
	IndexBufferManagerSPtr index_buffer_manager_sptr;
	IndirectBufferManagerSPtr indirect_buffer_manager_sptr;

	ShaderManager vertex_shader_manager{};
	ShaderManager fragment_shader_manager{};
	IndirectBufferManager indirect_buffer_manager{};

	DescriptorManager descriptor_manager{};

	RenderPassManager render_pass_manager{};

	PipelineManager graphics_pipeline_manager{};

	VkFramebuffer frame_buffers;

	unsigned int width{1024};
	unsigned int height{1024};
};
