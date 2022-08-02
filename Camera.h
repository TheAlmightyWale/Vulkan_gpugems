#pragma once
#include "Math.h"

class Camera
{
public:

	Camera(uint32_t screenWidth, uint32_t screenHeight);

	glm::mat4& GetViewProj();

private:
	glm::mat4 m_view;
	glm::mat4 m_proj;
	glm::mat4 m_vp;
};