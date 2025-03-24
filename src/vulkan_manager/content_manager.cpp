#include <content_manager.h>
#include <set>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
													VkDebugUtilsMessageTypeFlagsEXT messageType,
													const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
													void* pUserData)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
	}

	return VK_FALSE;
}

void ContentManager::setExtent(const VkExtent2D& extent)
{
	this->windowWidth = extent.width;
	this->windowHeight = extent.height;
}

void ContentManager::init()
{
	createWindow();

	if (!checkInstanceExtensionSupport())
	{
		throw std::runtime_error("Extenison not available!");
	}
	if (enableValidationLayers && !checkValidationLayerSupport())
	{
		throw std::runtime_error("Validation not available!");
	}

	createInstance();
	createSurface();
	createDebugMessenger();
	choosePhysicalDevice();
	createLogicalDevice();
}

void ContentManager::clear()
{
	if (this->enableValidationLayers)
	{
		auto function = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->instance,
																				   "vkDestroyDebugUtilsMessengerEXT");
		function(this->instance, this->debugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
	vkDestroyDevice(this->device, nullptr);
	vkDestroyInstance(this->instance, nullptr);

	glfwDestroyWindow(this->window);
	glfwTerminate();
}

void ContentManager::createInstance()
{
	/* Application Information */
	VkApplicationInfo applicationInfo{};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pApplicationName = "Rasterization";
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.pEngineName = "No Engine";
	applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.apiVersion = VK_API_VERSION_1_3;

	/* Vulkan Instance Information */
	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.flags = 0;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;

	/* Extension Information */
	auto extensions = getRequiredInstanceExtensions();
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

	/* Validation Layers Information */
	auto validationLayers = getRequiredValidationLayers();
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo{};
	if (this->enableValidationLayers)
	{
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

		debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
												   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
												   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
											   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
											   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugMessengerCreateInfo.pfnUserCallback = debugCallback;

		instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugMessengerCreateInfo;
	}
	else
	{
		instanceCreateInfo.enabledLayerCount = 0;
		instanceCreateInfo.ppEnabledLayerNames = nullptr;
		instanceCreateInfo.pNext = nullptr;
	}

	/* Create Instance */
	if (vkCreateInstance(&instanceCreateInfo, nullptr, &this->instance) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create instance!");
	}
}

void ContentManager::choosePhysicalDevice()
{
	/* Get the number of physical devices */
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(this->instance, &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}

	/* Get all physical devices */
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(this->instance, &deviceCount, devices.data());

	/* Find physical devices that meet requirements */
	for (const auto& device : devices)
	{
		if (isDeviceSuitable(device))
		{
			this->physicalDevice = device;
			break;
		}
	}
	if (this->physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Failed to find a suitable GPU!");
	}
}

bool ContentManager::isDeviceSuitable(VkPhysicalDevice physicalDevice)
{
	if (!checkDeviceExtensionSupport(physicalDevice))
	{
		return false;
	}

	/* Get the number of queue families */
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	/* Get queue family details */
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	int graphics = -1;
	int transfer = -1;
	int present = -1;
	int compute = -1;
	for (int i = 0; i < queueFamilyCount; i++)
	{
		if (graphics == -1 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			graphics = i;
		}
		if (transfer == -1 && queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			transfer = i;
		}
		if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			compute = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, this->surface, &presentSupport);
		if (present == -1 && presentSupport)
		{
			present = i;
		}
	}

	if (graphics != -1 && transfer != -1 && present != -1 && compute != -1)
	{
		this->graphicsFamily = static_cast<uint32_t>(graphics);
		this->transferFamily = static_cast<uint32_t>(transfer);
		this->presentFamily = static_cast<uint32_t>(present);
		this->computeFamily = static_cast<uint32_t>(compute);
		return true;
	}
	else
	{
		return false;
	}
}

std::vector<const char*> ContentManager::getRequiredInstanceExtensions()
{
	/* Get the required extensions for GLFW */
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (this->enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool ContentManager::checkInstanceExtensionSupport()
{
	/* Get the number of available extensions */
	uint32_t extensionCount;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	/* Get the attribute information of the extensions */
	std::vector<VkExtensionProperties> extensionProperties(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.data());

	for (const char* extensionName : getRequiredInstanceExtensions())
	{
		bool extensionFound = false;

		/* Check if the extension is in the available extensions */
		for (const auto& extensionProperty : extensionProperties)
		{
			if (strcmp(extensionName, extensionProperty.extensionName) == 0)
			{
				extensionFound = true;
				break;
			}
		}

		if (!extensionFound)
		{
			return false;
		}
	}

	return true;
}

std::vector<const char*> ContentManager::getRequiredValidationLayers()
{
	std::vector<const char*> validationLayers{};
	validationLayers.push_back("VK_LAYER_KHRONOS_validation");
	return validationLayers;
}

void ContentManager::createWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	this->window = glfwCreateWindow(this->windowWidth, this->windowHeight, "Vulkan", nullptr, nullptr);
}

bool ContentManager::checkValidationLayerSupport()
{
	/* Get the number of available validation layers */
	uint32_t layerPropertyCount;
	vkEnumerateInstanceLayerProperties(&layerPropertyCount, nullptr);

	/* Get the attribute information of the validation layer */
	std::vector<VkLayerProperties> layerProperties(layerPropertyCount);
	vkEnumerateInstanceLayerProperties(&layerPropertyCount, layerProperties.data());

	for (const char* layerName : getRequiredValidationLayers())
	{
		bool layerFound = false;

		/* Check if the validation layer is in the available validation layers */
		for (const auto& layerProperties : layerProperties)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

std::vector<const char*> ContentManager::getRequiredDeviceExtensions()
{
	std::vector<const char*> deviceExtensions{};
	if (this->enablePathTracing)
	{
		deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
		deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
		deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	}
	deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	return deviceExtensions;
}

bool ContentManager::checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
{
	/* Get the number of extensions supported by the physical device */
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	/* Get all extensions supported by the physical device */
	std::vector<VkExtensionProperties> extensionProperties(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensionProperties.data());

	for (const char* extensionName : getRequiredDeviceExtensions())
	{
		bool extensionFound = false;

		/* Check if the extension is in the available extensions */
		for (const auto& extensionProperty : extensionProperties)
		{
			if (strcmp(extensionName, extensionProperty.extensionName) == 0)
			{
				extensionFound = true;
				break;
			}
		}

		if (!extensionFound)
		{
			return false;
		}
	}

	return true;
}

void ContentManager::createDebugMessenger()
{
	if (!this->enableValidationLayers)
	{
		return;
	}

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;

	auto function =
		(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->instance, "vkCreateDebugUtilsMessengerEXT");
	if (function != nullptr)
	{
		function(this->instance, &createInfo, nullptr, &this->debugMessenger);
	}
	else
	{
		throw std::runtime_error("Failed to set up debug messenger!");
	}
}

void ContentManager::createLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {
		this->graphicsFamily, this->transferFamily, this->presentFamily, this->computeFamily};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		/* Information about the queues used by the logical device */
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.pNext = nullptr;
		queueCreateInfo.flags = 0;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	/* Physical device characteristic information used by logical devices */
	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	/* Logical device creation information */
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	auto extension = getRequiredDeviceExtensions();
	createInfo.enabledExtensionCount = extension.size();
	createInfo.ppEnabledExtensionNames = extension.data();
	createInfo.pEnabledFeatures = &deviceFeatures;

	auto validationLayers = getRequiredValidationLayers();
	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}

	/* Creating a Logical Device */
	if (vkCreateDevice(this->physicalDevice, &createInfo, nullptr, &this->device) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create logical device!");
	}

	/* Get Queues */
	vkGetDeviceQueue(this->device, this->graphicsFamily, 0, &this->graphicsQueue);
	vkGetDeviceQueue(this->device, this->transferFamily, 0, &this->transferQueue);
	vkGetDeviceQueue(this->device, this->presentFamily, 0, &this->presentQueue);
	vkGetDeviceQueue(this->device, this->computeFamily, 0, &this->computeQueue);
}

void ContentManager::createSurface()
{
	if (glfwCreateWindowSurface(this->instance, this->window, nullptr, &this->surface) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface!");
	}
}
