#include "ModelLoader.h"
#include "lib/objparser/objparser.h"
#include "Exceptions.h"
//TODO turn mesh optimizer into a library
#include "lib/meshoptimizer/src/meshoptimizer.h"

#include "GfxDevice.h"
#include "GfxBuffer.h"

MeshPtr_t ModelLoader::LoadModel(GfxDevicePtr_t const pDevice, std::string const& filePath)
{
	ObjFile parsedObj;
	if (!objParseFile(parsedObj, filePath.c_str()))
	{
		throw InvalidStateException("File not found: " + filePath);
	}

	size_t const indexCount = parsedObj.f_size / 3; //face count / triangles

	std::vector<Vertex> vertices(indexCount);

	for (size_t i = 0; i < indexCount; ++i)
	{
		Vertex& v = vertices[i];
		uint32_t vIndex = parsedObj.f[i * 3 + 0];
		uint32_t vTextureIndex = parsedObj.f[i * 3 + 1];
		uint32_t vNormalIndex = parsedObj.f[i * 3 + 2];

		v.vx = parsedObj.v[vIndex * 3 + 0];
		v.vy = parsedObj.v[vIndex * 3 + 1];
		v.vz = parsedObj.v[vIndex * 3 + 2];

		v.nx = vNormalIndex < 0 ? 0.f : parsedObj.vn[vNormalIndex * 3 + 0];
		v.ny = vNormalIndex < 0 ? 0.f : parsedObj.vn[vNormalIndex * 3 + 1];
		v.nz = vNormalIndex < 0 ? 1.f : parsedObj.vn[vNormalIndex * 3 + 2];

		v.u = parsedObj.vt[vTextureIndex * 3 + 0];
		v.v = parsedObj.vt[vTextureIndex * 3 + 1];
	}

	std::vector<uint32_t> remap(indexCount);
	size_t totalVertices = meshopt_generateVertexRemap(remap.data(), nullptr, indexCount, vertices.data(), indexCount, sizeof(Vertex));

	std::vector<Vertex> remappedVertices;
	std::vector<uint32_t> indices;
	remappedVertices.resize(totalVertices);
	indices.resize(indexCount);

	meshopt_remapVertexBuffer(remappedVertices.data(), vertices.data(), indexCount, sizeof(Vertex), remap.data());
	meshopt_remapIndexBuffer(indices.data(), nullptr, indexCount, remap.data());

	MeshPtr_t pMesh = std::make_shared<Mesh>();
	pMesh->vertexBuffer = pDevice->CreateBuffer(remappedVertices.size() * sizeof(Vertex), vk::BufferUsageFlagBits::eVertexBuffer);
	pMesh->indexBuffer = pDevice->CreateBuffer(indices.size() * sizeof(uint32_t), vk::BufferUsageFlagBits::eIndexBuffer);

	memcpy(pMesh->vertexBuffer.m_pData, remappedVertices.data(), pMesh->vertexBuffer.m_dataSize);
	memcpy(pMesh->indexBuffer.m_pData, indices.data(), pMesh->indexBuffer.m_dataSize);

	return pMesh;
}
