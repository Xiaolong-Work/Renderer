#pragma once

#include <fstream>

#include <context_manager.h>

class ShaderManager
{
public:
	ShaderManager() = default;
	ShaderManager(const ContextManagerSPtr& context_manager_sptr);

	void init();
	void clear();

	void setShaderName(const std::string& name);

	VkShaderModule module;

protected:
	void loadShaderCode();

	void createShaderModule();

private:
	ContextManagerSPtr context_manager_sptr;
	std::vector<char> code;
	std::string name;
};