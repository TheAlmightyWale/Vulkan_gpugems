#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>


constexpr glm::vec3 BASE_FORWARD = glm::vec3(0.0f, 0.0f, 1.0f);
constexpr glm::vec3 BASE_BACKWARD = glm::vec3(0.0f, 0.0f, -1.0f);
constexpr glm::vec3 BASE_LEFT = glm::vec3(-1.0f, 0.0f, 0.0f);
constexpr glm::vec3 BASE_RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);