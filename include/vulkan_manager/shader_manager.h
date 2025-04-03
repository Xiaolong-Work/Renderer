#pragma once

#include <fstream>

#include <context_manager.h>

class ShaderManager
{
public:
	ShaderManager() = default;
	ShaderManager(const ContextManagerSPtr& pContentManager);

	void init();
	void clear();

	void setShaderName(const std::string& shaderName);

	VkShaderModule module;

protected:
	void loadShaderCode();

	void createShaderModule();

private:
	ContextManagerSPtr pContentManager;
	std::vector<char> code;
	std::string shaderName;
};