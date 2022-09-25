#pragma once
#include "Window.h"

struct ButtonState
{
	bool isPressed;
};

struct ControllerInput
{
	union
	{
		ButtonState Buttons[4];
		struct
		{
			ButtonState MoveUp;
			ButtonState MoveDown;
			ButtonState MoveLeft;
			ButtonState MoveRight;
		};
	};
};

class InputManager {
public:
	InputManager(Window const& window) noexcept;

	ControllerInput const& GetState() const noexcept;

	//Key and action values are based off of GLFW kley and action definitions
	void HandleKeyEvent(int key, int action) noexcept;

private:
	ControllerInput inputState;
};