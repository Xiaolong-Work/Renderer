#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <stdexcept>
#include <vector>

#include <buffer_manager.h>
#include <command_manager.h>
#include <content_manager.h>
#include <texture_manager.h>

class DescriptorManager
{
public:
    DescriptorManager() = default;
    DescriptorManager(const ContentManagerSPtr& pContentManager,
                      const UniformBufferManagerSPtr& pUniformBufferManager,
                      const TextureManagerSPtr& pTextureManager);

    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();
    void init();
    void clear();

    VkDescriptorSetLayout setLayout;
    VkDescriptorPool pool;
    std::vector<VkDescriptorSet> sets;

private:
    ContentManagerSPtr pContentManager;
    UniformBufferManagerSPtr pUniformBufferManager;
    TextureManagerSPtr pTextureManager;
};
