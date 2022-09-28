#pragma once
#include "Math.h"

struct ControllerInput;

class Camera
{
public:

	Camera(uint32_t screenWidth, uint32_t screenHeight);

	void Process(ControllerInput const& inputState, float deltaTime );

	glm::mat4 const& GetViewProj() const noexcept;

private:
	glm::vec3 m_target;
	glm::vec3 m_position;
	glm::vec3 m_up;
	glm::mat4 m_proj;
	glm::mat4 m_vp;
};