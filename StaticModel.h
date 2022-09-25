#pragma once
#include "Math.h"
#include "Mesh.h"
#include "GfxBuffer.h"

struct ObjectData
{
	glm::mat4 transform;
};

class StaticModel {
public:
	StaticModel(GfxDevicePtr_t const pDevice, MeshPoolPtr_t meshPool, std::string const& modelFilePath);

	void SetPosition(glm::vec3 const& position);
	void SetRotation(float degrees, glm::vec3 const& axis);
	void SetScale(glm::vec3 const& scale);

	glm::mat4 const& GetTransform();

	GfxBuffer const& GetVertexBuffer();
	GfxBuffer const& GetIndexBuffer();

private:
	MeshPtr_t m_pMesh;
	ObjectData m_transform;
};

using StaticModelPtr_t = std::shared_ptr<StaticModel>;