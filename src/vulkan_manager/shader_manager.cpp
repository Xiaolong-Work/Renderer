#include <shader_manager.h>

ShaderManager::ShaderManager(const ContentManagerSPtr& pContentManager)
{
	this->pContentManager = pContentManager;
}

void ShaderManager::init()
{
	loadShaderCode();
	createShaderModule();
}

void ShaderManager::clear()
{
	vkDestroyShaderModule(pContentManager->device, this->module, nullptr);
}

void ShaderManager::setShaderName(const std::string& shaderName)
{
	this->shaderName = shaderName;
}

void ShaderManager::loadShaderCode()
{
	/* Read the file from the end to get the file size */
	std::ifstream file(std::string(SHADERS_DIR) + this->shaderName, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open shader file!");
	}
	size_t fileSize = (size_t)file.tellg();

	/* Apply for buffer */
	this->code.resize(fileSize);

	/* Read file contents */
	file.seekg(0);
	file.read(this->code.data(), fileSize);

	file.close();
}

void ShaderManager::createShaderModule()
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.codeSize = this->code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	if (vkCreateShaderModule(pContentManager->device, &createInfo, nullptr, &this->module) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create shader module!");
	}

	this->code.clear();
}