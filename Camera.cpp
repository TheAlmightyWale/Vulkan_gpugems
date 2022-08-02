#include "Camera.h"

Camera::Camera(uint32_t screenWidth, uint32_t screenHeight)
{
    //hardcoded camera for now
	glm::vec3 camPos = { 0.0f,0.0f, -2.0f };
	m_view = glm::translate(glm::identity<glm::mat4>(), camPos);
	float aspectRatio = (float)screenWidth / (float)screenHeight;
	m_proj = glm::perspective(glm::radians(70.0f), aspectRatio, 0.1f, 100.0f);
	m_vp = m_proj * m_view;
}

glm::mat4& Camera::GetViewProj()
{
	return m_vp;
}
