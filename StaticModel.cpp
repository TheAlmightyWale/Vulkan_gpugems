#include "StaticModel.h"
#include "GfxDevice.h"
#include "ModelLoader.h"

//TODO this gets waay cleaner with ECS or other component management so we don't have to talk to object processor directly
StaticModel::StaticModel(GfxDevicePtr_t const pDevice, MeshPoolPtr_t meshPool, std::string const& modelFilePath, ObjectProcessorPtr_t pObjectProcessor)
	: m_transform(pObjectProcessor->AddStaticMesh(ObjectData{glm::identity<glm::mat4>()}))
{
	if (!meshPool->contains(modelFilePath))
	{
		MeshPtr_t pMesh = ModelLoader::LoadModel(pDevice, modelFilePath);
		meshPool->emplace(modelFilePath, pMesh);
	}
	m_pMesh = meshPool->at(modelFilePath);
}

void StaticModel::SetPosition(glm::vec3 const& position)
{
	m_transform.transform = glm::translate(glm::identity<glm::mat4>(), position);
}

void StaticModel::SetRotation(float degrees, glm::vec3 const& axis)
{
	m_transform.transform = glm::rotate(glm::identity<glm::mat4>(), glm::radians(degrees), axis);
}

void StaticModel::SetScale(glm::vec3 const& scale)
{
	m_transform.transform = glm::scale(glm::identity<glm::mat4>(), scale);
}

glm::mat4 const& StaticModel::GetTransform()
{
	return m_transform.transform;
}

GfxBuffer const& StaticModel::GetVertexBuffer()
{
	return m_pMesh->vertexBuffer;
}

GfxBuffer const& StaticModel::GetIndexBuffer()
{
	return m_pMesh->indexBuffer;
}
