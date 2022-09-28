#pragma once
#include "Window.h"
#include "GfxFwdDecl.h"

class InputManager;
class ObjectProcessor;

//App is responsible for managing window lifetimes and the main event loop
class App
{
public:
	App(std::string const& appName);
	~App();

	void Start();
	void Process();

	bool ShouldQuit() noexcept;

private:
	WindowPtr_t m_pWindow;
	std::string const m_appName;
	std::shared_ptr<GfxEngine> m_pGfxEngine;
	std::shared_ptr<InputManager> m_pInputManager;
	std::shared_ptr<ObjectProcessor> m_pObjectProcessor;
};

