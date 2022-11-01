#include "InputManager.h"
#include "Exceptions.h"

void ProcessButtonState(int glfwButtonAction, ButtonState& processedState) noexcept
{
	if (glfwButtonAction == GLFW_PRESS)
	{
		processedState.isPressed = true;
	}
	else if (glfwButtonAction == GLFW_RELEASE)
	{
		processedState.isPressed = false;
	}
}

void InputManager::HandleKeyEvent(int key, int action) noexcept
{
	//Just taking WASD keyboard input for now
	switch (key)
	{
	case GLFW_KEY_W:
		ProcessButtonState(action, inputState.MoveUp);
		break;
	case GLFW_KEY_A:
		ProcessButtonState(action, inputState.MoveLeft);
		break;
	case GLFW_KEY_S:
		ProcessButtonState(action, inputState.MoveDown);
		break;
	case GLFW_KEY_D:
		ProcessButtonState(action, inputState.MoveRight);
		break;
	default:
		break;
	}
}

void KeyEventCallback(GLFWwindow* pWindow, int key, int scancode, int action, int mods)
{
	InputManager* pInputManager = static_cast<InputManager*>(glfwGetWindowUserPointer(pWindow));

	if (!pInputManager)
	{
		throw InvalidStateException("No input manager available to ahndle keyboard events");
	}
	
	pInputManager->HandleKeyEvent(key, action);
}

InputManager::InputManager(Window const& window) noexcept
	: inputState()
{
	//Is InputManager the only user data we need out of glfw?
	glfwSetWindowUserPointer(window.Get(), this);

	glfwSetKeyCallback(window.Get(), KeyEventCallback);
}

ControllerInput const& InputManager::GetState() const noexcept
{
	return inputState;
}
