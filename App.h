#pragma once
#include "Window.h"
#include "GfxFwdDecl.h"

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
	bool m_bShouldQuit;
	std::string const m_appName;
	std::shared_ptr<GfxEngine> m_pGfxEngine;
};

