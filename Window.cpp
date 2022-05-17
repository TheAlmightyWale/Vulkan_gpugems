#include "Window.h"
#include "Exceptions.h"

Window::Window(WindowDimensions windowSize, std::string const& windowName)
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
	int width, height;
	glfwGetWindowSize(m_pWindow, &width, &height);
	return std::make_tuple((uint32_t)width, (uint32_t)height);
}

bool Window::ShouldClose() const noexcept
{
	return glfwWindowShouldClose(m_pWindow);
}