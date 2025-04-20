#include <shadow_map_manager.h>

ShadowMapManager::ShadowMapManager(const ContextManagerSPtr& context_manager_sptr,
								   const CommandManagerSPtr& command_manager_sptr,
								   const VertexBufferManagerSPtr& vertex_buffer_manager_sptr,
								   const IndexBufferManagerSPtr& index_buffer_manager_sptr,
								   const IndirectBufferManagerSPtr& indirect_buffer_manager_sptr,
								   const StorageBufferManagerSPtr& model_matrix_manager_sptr,
								   const TextureManagerSPtr& texture_manager_sptr,
								   const StorageBufferManagerSPtr material_index_manager_sptr,
								   const StorageBufferManagerSPtr material_ssbo_manager)
{
	this->context_manager_sptr = context_manager_sptr;
	this->command_manager_sptr = command_manager_sptr;
	this->vertex_buffer_manager_sptr = vertex_buffer_manager_sptr;
	this->index_buffer_manager_sptr = index_buffer_manager_sptr;
	this->indirect_buffer_manager_sptr = indirect_buffer_manager_sptr;
	this->model_matrix_manager_sptr = model_matrix_manager_sptr;
	this->texture_manager_sptr = texture_manager_sptr;
	this->material_index_manager_sptr = material_index_manager_sptr;
	this->material_ssbo_manager_sptr = material_ssbo_manager;
}

void ShadowMapManager::init()
{
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

	this->direction_light_shadow_map_manager =
		DirectionLightShadowMapManager(this->context_manager_sptr, this->command_manager_sptr);
	this->direction_light_shadow_map_manager.light_number = this->direction_lights.size();
	if (!this->direction_lights.empty())
	{
		this->direction_light_shadow_map_manager.init();
	}
	

	this->render_pass_manager = RenderPassManager(this->context_manager_sptr, this->command_manager_sptr);
	this->render_pass_manager.init();
	this->createFrameBuffer();

	this->point_pipeline_manager = PipelineManager(context_manager_sptr);
	this->setupGraphicsPipelines();

	this->point_light_manager = StorageBufferManager(context_manager_sptr, command_manager_sptr);
	this->point_light_manager.setData(
		this->point_lights.data(), sizeof(PointLight) * this->point_lights.size(), this->point_lights.size());
	this->point_light_manager.init();

	this->renderShadowMap();
}

void ShadowMapManager::clear()
{
	this->point_light_manager.clear();

	this->point_pipeline_manager.clear();

	this->render_pass_manager.clear();

	this->direction_light_shadow_map_manager.clear();

	this->point_light_shadow_map_manager.clear();

	this->descriptor_manager.clear();

	this->mvp_ssbo_manager.clear();

	this->indirect_buffer_manager.clear();

	vkDestroyFramebuffer(context_manager_sptr->device, this->frame_buffers, nullptr);
}