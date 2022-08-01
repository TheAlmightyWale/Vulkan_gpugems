#include "GfxApiInstance.h"
#include "Logger.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

static VKAPI_ATTR VkBool32 VKAPI_CALL debugMessageFunc(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
	void* /*pUserData*/
) {
	std::string message = std::string("Vk: ") + pCallbackData->pMessage;

	auto severityEnum = vk::DebugUtilsMessageSeverityFlagBitsEXT(messageSeverity);

	switch (severityEnum) {
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
		SPDLOG_DEBUG(message);
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
		SPDLOG_INFO(message);
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
		SPDLOG_WARN(message);
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
		SPDLOG_ERROR(message);
		break;
	}

	return VK_FALSE;
}

//TODO move out to engine?
std::vector<const char*> const k_instanceLayers{
#ifdef _DEBUG
	"VK_LAYER_KHRONOS_validation",
#endif // NDEBUG
};

std::vector<const char*> const k_instanceExtensions{
#ifdef _DEBUG
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif // NDEBUG
	VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
};


std::vector<char const*> GetGlfwExtensions() {
	uint32_t numGlfwExtensions = 0;
	const char** glfwExtensionNames;

	glfwExtensionNames = glfwGetRequiredInstanceExtensions(&numGlfwExtensions);

	//Does this constructor resize the vector as well?
	std::vector<char const*> extensions(glfwExtensionNames, glfwExtensionNames + numGlfwExtensions);

	//TODO is move needed here? does it fall back to copy anyway?
	return extensions;
}

GfxApiInstance::GfxApiInstance(std::string const& applicationName, uint32_t appVersion, std::string const& engineName, uint32_t const engineVersion, uint32_t const vulkanVersion )
	: m_pContext(std::make_shared<vk::raii::Context>())
	, m_pInstance()
	, m_debugMessenger(nullptr)
{

	SPDLOG_INFO("Initializing Gfx Api");

	vk::ApplicationInfo const appInfo(
		applicationName.c_str(),
		appVersion,
		engineName.c_str(),
		engineVersion,
		vulkanVersion);

	std::vector<char const*> enabledLayers = k_instanceLayers;
	std::vector<char const*> enabledExtensions = GetGlfwExtensions();
	enabledExtensions.insert(enabledExtensions.end(), k_instanceExtensions.begin(), k_instanceExtensions.end());

	vk::InstanceCreateInfo instanceCreateInfo({}/*flags*/, &appInfo, enabledLayers, enabledExtensions);

	vk::DynamicLoader dl;
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	SPDLOG_INFO("Creating instance.");
	SPDLOG_INFO("Enabled Layers: {}", enabledLayers);
	SPDLOG_INFO("Enabled Extensions: {}", enabledExtensions);

	m_pInstance = std::make_shared<vk::raii::Instance>(*m_pContext, instanceCreateInfo);

	SPDLOG_INFO("Loading Vulkan functions");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(**m_pInstance);
	
#ifdef DEBUG
	vk::DebugUtilsMessageSeverityFlagsEXT const severityFlags(
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
	);
	vk::DebugUtilsMessageTypeFlagsEXT const messageTypeFlags(
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
		vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

	m_debugMessenger = m_pInstance->createDebugUtilsMessengerEXT(
		vk::DebugUtilsMessengerCreateInfoEXT(
			{} /*create flags */,
			severityFlags,
			messageTypeFlags,
			&debugMessageFunc
		)
	);
#endif // DEBUG
}

GfxApiInstance::~GfxApiInstance()
{
}
