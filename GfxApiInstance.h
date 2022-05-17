#pragma once
#include "Window.h"
#include "GfxFwdDecl.h"

class GfxApiInstance
{
public:
	GfxApiInstance(
		std::string const& applicationName,
		uint32_t appVersion,
		std::string const& engineName,
		uint32_t const engineVersion,
		uint32_t const vulkanVersion);
	~GfxApiInstance();

	vk::raii::Instance const& GetInstance() const { return *m_pInstance.get();}

private:
	ContextPtr_t m_pContext;
	InstancePtr_t m_pInstance;
	vk::raii::DebugUtilsMessengerEXT m_debugMessenger;
};

using GfxApiInstancePtr_t = std::shared_ptr<GfxApiInstance>;

