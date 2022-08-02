#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include <string>
#include <memory>
#include <tuple>

using WindowDimensions = std::tuple<uint32_t, uint32_t>;

//Window abstracts glfw window operations and lifetime
class Window
{
public:
	Window(WindowDimensions windowSize, std::string const& windowName);
	virtual ~Window();

	WindowDimensions GetWindowSize() const;
	uint32_t GetWindowWidth() const;
	uint32_t GetWindowHeight() const;
	bool ShouldClose() const noexcept;

	GLFWwindow* Get() const noexcept { return m_pWindow; }

private:
	GLFWwindow* m_pWindow;
	WindowDimensions m_windowSize;
};

using WindowPtr_t = std::shared_ptr<Window>;

