#include <shader_manager.h>

ShaderManager::ShaderManager(const ContextManagerSPtr& context_manager_sptr)
{
	this->context_manager_sptr = context_manager_sptr;
}

void ShaderManager::init()
{
	loadShaderCode();
	createShaderModule();
}

void ShaderManager::clear()
{
	vkDestroyShaderModule(context_manager_sptr->device, this->module, nullptr);
}

void ShaderManager::setShaderName(const std::string& name)
{
	this->name = name;
}

void ShaderManager::loadShaderCode()
{
	/* Read the file from the end to get the file size */
	std::ifstream file(std::string(SHADERS_DIR) + this->name, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open shader file!");
	}
	size_t size = (size_t)file.tellg();

	/* Apply for buffer */
	this->code.resize(size);

	/* Read file contents */
	file.seekg(0);
	file.read(this->code.data(), size);

	file.close();
}

void ShaderManager::createShaderModule()
{
	VkShaderModuleCreateInfo create_information{};
	create_information.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_information.pNext = nullptr;
	create_information.flags = 0;
	create_information.codeSize = this->code.size();
	create_information.pCode = reinterpret_cast<const uint32_t*>(code.data());

	if (vkCreateShaderModule(context_manager_sptr->device, &create_information, nullptr, &this->module) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create shader module!");
	}

	this->code.clear();
}