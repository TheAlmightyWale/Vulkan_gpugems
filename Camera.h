#pragma once
#include "Math.h"

struct ControllerInput;

struct CameraShaderData
{
	glm::mat4 viewProj;
};

class Camera
{
public:

	Camera(uint32_t screenWidth, uint32_t screenHeight);

	void Process(ControllerInput const& inputState, float deltaTime );

	glm::mat4 const& GetViewProj() const noexcept;
	glm::vec3 const& GetPosition() const noexcept;

private:
	glm::vec3 m_target;
	glm::vec3 m_position;
	glm::vec3 m_up;
	glm::mat4 m_proj;
	CameraShaderData m_cameraShaderData;
};