#pragma once

#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <content_manager.h>

class ShaderManager
{
public:
	ShaderManager() = default;
	ShaderManager(const ContentManagerSPtr& pContentManager);

	void init();
	void clear();

	void setShaderName(const std::string& shaderName);

	VkShaderModule module;

protected:
	void loadShaderCode();

	void createShaderModule();

private:
	ContentManagerSPtr pContentManager;
	std::vector<char> code;
	std::string shaderName;
};