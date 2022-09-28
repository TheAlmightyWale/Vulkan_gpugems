#include "ObjectProcessor.h"
#include "Camera.h"
#include "Logger.h"
#include "InputManager.h"
#include "StaticModel.h"

ObjectProcessor::ObjectProcessor(std::shared_ptr<InputManager> pInputManager)
	: m_pInputManager(pInputManager)
	, m_pCamera(nullptr) //TODO how do we get camera to here?
	, m_meshTransforms()
{
	m_meshTransforms.reserve(k_maxModelTransforms);
}

void ObjectProcessor::SetCamera(std::shared_ptr<Camera> const& pCamera)
{
	m_pCamera = pCamera;
}

ObjectData& ObjectProcessor::AddStaticMesh(ObjectData transform)
{
	m_meshTransforms.push_back(std::move(transform));

	return m_meshTransforms.back();
}

//TODO this is pretty much just running through all first 8 3-bit permutations
// 000 -> 111. Could do some fancy bit stuff if we wanna
std::vector<glm::vec3> const k_rotationPermutations = {
	{0.0f, 0.0f, 0.0f},
	{0.0f, 0.0f, 1.0f},
	{0.0f, 1.0f, 0.0f},
	{0.0f, 1.0f, 1.0f},
	{1.0f, 0.0f, 0.0f},
	{1.0f, 0.0f, 1.0f},
	{1.0f, 1.0f, 0.0f},
	{1.0f, 1.0f, 1.0f},
};

void ObjectProcessor::ProcessObjects(float deltaTime)
{
	for (size_t i = 0; i < m_meshTransforms.size(); ++i)
	{
		glm::mat4& transform = m_meshTransforms.at(i).transform;

		// Each mesh rotates based on it's index
		constexpr float k_rotationDegreesPerSecond = 30.0f;
		size_t index = i % k_rotationPermutations.size();
		if (index != 0) { //special case where we do not rotate otherwise would be rotating by zero-axis
			transform = glm::rotate(transform, glm::radians(k_rotationDegreesPerSecond * deltaTime), k_rotationPermutations.at(i % k_rotationPermutations.size()));
		}
	}

	if (m_pInputManager)
	{
		if (m_pCamera) m_pCamera->Process(m_pInputManager->GetState(), deltaTime);
	}
	else
	{
		SPDLOG_WARN("No InputManager available while processing objects");
	}
}

//TODO how do we want to handle object destruction?
//		Does each processor just destroy the components it is interested in and we assume
//		all processors will be alive for the the lifespan of the program?
ObjectProcessor::~ObjectProcessor()
{
}
