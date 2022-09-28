#include "App.h"
#include "Logger.h"
#include "GfxEngine.h"
#include "InputManager.h"
#include "ObjectProcessor.h"

uint32_t const k_appVersion = 1;

App::App(std::string const& appName)
	: m_appName(appName)
	, m_pGfxEngine(nullptr)
	, m_pInputManager(nullptr)
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
		m_pInputManager = std::make_shared<InputManager>(*m_pWindow);
		m_pObjectProcessor = std::make_shared<ObjectProcessor>(m_pInputManager);
		m_pGfxEngine = std::make_shared<GfxEngine>(m_appName, k_appVersion, m_pWindow, m_pObjectProcessor);
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
	return m_pWindow->ShouldClose();
}

void App::Process() {
	//Topmost error handler
	try
	{
		glfwPollEvents();
		m_pObjectProcessor->ProcessObjects(0.03f); //TODO actual deltatime getting
		m_pGfxEngine->Render();
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
