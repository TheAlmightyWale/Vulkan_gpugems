#include "Camera.h"
#include "InputManager.h"

constexpr float k_cameraMoveSpeed = 1.0f;

Camera::Camera(uint32_t screenWidth, uint32_t screenHeight)
{
    //hardcoded camera for now
	m_position = { 2.0f, 4.0f, -6.0f };
	m_up = { 0.0f, 1.0f, 0.0f };
	m_target = { 1.0f, 4.0f, 2.0f };

	float const aspectRatio = (float)screenWidth / (float)screenHeight;
	m_proj = glm::perspective(glm::radians(70.0f), aspectRatio, 0.1f, 100.0f);
	m_cameraShaderData.viewProj = m_proj * glm::lookAt(m_position, m_target, m_up);
}

void Camera::Process(ControllerInput const& inputState, float deltaTime)
{
	glm::vec3 movement(0.0f);

	if (inputState.MoveUp.isPressed) {
		movement += k_cameraMoveSpeed * BASE_FORWARD * deltaTime;
	}

	if (inputState.MoveDown.isPressed) {
		movement += k_cameraMoveSpeed * BASE_BACKWARD * deltaTime;
	}

	if (inputState.MoveRight.isPressed) {
		movement += k_cameraMoveSpeed * BASE_RIGHT * deltaTime;
	}

	if (inputState.MoveLeft.isPressed) {
		movement += k_cameraMoveSpeed * BASE_LEFT * deltaTime;
	}

	m_position += movement;
	//TODO update target as we move? Could also just have camera circle around the center point?

	m_cameraShaderData.viewProj = m_proj * glm::lookAt(m_position, m_target, m_up);
}

glm::mat4 const& Camera::GetViewProj() const noexcept
{
	return m_cameraShaderData.viewProj;
}
