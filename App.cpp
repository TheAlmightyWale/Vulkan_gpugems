#include "App.h"
#include "Logger.h"
#include "GfxEngine.h"

uint32_t const k_appVersion = 1;

App::App(std::string const& appName)
	: m_appName(appName)
	, m_bShouldQuit(false)
	, m_pGfxEngine(nullptr)
{
	Logger::InitLogger();
	glfwInit();
}

App::~App() {
	glfwTerminate();
}

void App::Start() {
	//Topmost error handler
	try
	{
		WindowDimensions size = { 800, 600 };
		m_pWindow = std::make_shared<Window>(size, m_appName);

		m_pGfxEngine = std::make_shared<GfxEngine>(m_appName, k_appVersion, m_pWindow);
	}
	catch (std::exception const& err)
	{
		SPDLOG_ERROR("Error: {}", err.what());
		exit(-1);
	}
	catch (...)
	{
		SPDLOG_ERROR("An unknown error has occurred");
		exit(-1);
	}
}

bool App::ShouldQuit() noexcept{
	return m_bShouldQuit;
}

void App::Process() {
	//Topmost error handler
	try
	{
		while (!m_pWindow->ShouldClose())
		{
			glfwPollEvents();
			m_pGfxEngine->Render();
		}
	}
	catch (std::exception& err)
	{
		SPDLOG_ERROR("Error: {}", err.what());
		exit(-1);
	}
	catch (...)
	{
		SPDLOG_ERROR("An unknown error has occurred");
		exit(-1);
	}
}
