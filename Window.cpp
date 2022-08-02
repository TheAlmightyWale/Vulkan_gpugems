#include "Window.h"
#include "Exceptions.h"

Window::Window(WindowDimensions windowSize, std::string const& windowName)
	: m_windowSize(windowSize)
	, m_pWindow(nullptr)
{
	int width, height;
	std::tie(width, height) = windowSize;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_pWindow = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);

	if (!m_pWindow)
	{
		const char* errorCharPtr;
		int errorCode = glfwGetError(&errorCharPtr);
		//Copy error description so it is not overwritten;
		std::string errorDescription(errorCharPtr);

		throw InitializationException(std::to_string(errorCode) + errorDescription);
	}
}

Window::~Window() 
{
	glfwDestroyWindow(m_pWindow);
}

WindowDimensions Window::GetWindowSize() const
{
	return m_windowSize;
}

uint32_t Window::GetWindowWidth() const
{
	return std::get<0>(m_windowSize);
}

uint32_t Window::GetWindowHeight() const
{
	return std::get<1>(m_windowSize);
}

bool Window::ShouldClose() const noexcept
{
	return glfwWindowShouldClose(m_pWindow);
}