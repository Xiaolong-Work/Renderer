#include <vulkan_utils.h>


DrawFrame::DrawFrame()
{

}

DrawFrame::~DrawFrame()
{

}

void DrawFrame::init()
{
	this->content_manager.enable_window_resize = false;
	this->content_manager.init();
	auto content_manager_sptr = std::make_shared<ContextManager>(this->content_manager);

	this->command_manager = CommandManager(content_manager_sptr);
	this->command_manager.init();
	auto command_manager_sptr = std::make_shared<CommandManager>(this->command_manager);

	this->swap_chain_manager = SwapChainManager(content_manager_sptr, command_manager_sptr);
	this->swap_chain_manager.init();
	auto swap_chain_manager_sptr = std::make_shared<SwapChainManager>(this->swap_chain_manager);

	this->render_pass_manager = RenderPassManager(content_manager_sptr, swap_chain_manager_sptr, command_manager_sptr);
	this->render_pass_manager.init();

	this->vertex_shader_manager = ShaderManager(content_manager_sptr);
	this->vertex_shader_manager.setShaderName("draw_single_frame_vert.spv");
	this->vertex_shader_manager.init();

	this->fragment_shader_manager = ShaderManager(content_manager_sptr);
	this->fragment_shader_manager.setShaderName("draw_single_frame_frag.spv");
	this->fragment_shader_manager.init();

	this->texture_manager = TextureManager(content_manager_sptr, command_manager_sptr);
	this->texture_manager.init();
	this->texture_manager.createEmptyTexture();

	this->descriptor_manager = DescriptorManager(content_manager_sptr);
	this->setupDescriptor();

	this->graphics_pipeline_manager = PipelineManager(content_manager_sptr);
	this->setupGraphicsPipelines();

	this->buffer_manager = StagingBufferManager(content_manager_sptr, command_manager_sptr);

	createSyncObjects();
}

void DrawFrame::clear()
{
	vkDestroySemaphore(content_manager.device, renderFinishedSemaphores, nullptr);
	vkDestroySemaphore(content_manager.device, imageAvailableSemaphores, nullptr);
	vkDestroyFence(content_manager.device, inFlightFences, nullptr);

	this->graphics_pipeline_manager.clear();

	this->descriptor_manager.clear();

	this->texture_manager.clear();

	this->fragment_shader_manager.clear();

	this->vertex_shader_manager.clear();

	this->render_pass_manager.clear();

	this->swap_chain_manager.clear();

	this->command_manager.clear();

	this->content_manager.clear();
}

void DrawFrame::draw()
{
	int count = 0;
	float sum = 0.0f;
	while (!glfwWindowShouldClose(this->content_manager.window))
	{
		auto begin = std::chrono::system_clock::now();
		render();
		present();
		glfwPollEvents();
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

	vkDeviceWaitIdle(this->content_manager.device);
}
