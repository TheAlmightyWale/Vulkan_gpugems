#include "ModelLoader.h"
#include "lib/objparser/objparser.h"
#include "Exceptions.h"
//TODO turn mesh optimizer into a library
#include "lib/meshoptimizer/src/meshoptimizer.h"

#include "Cube.h"

Mesh ModelLoader::LoadCube()
{
	Mesh cube;
	for (uint32_t i = 0; i < k_cubeVertexValuesCount; i += 3)
	{
		Vertex v{};
		v.vx = k_cubeVertices[i + 0];
		v.vy = k_cubeVertices[i + 1];
		v.vz = k_cubeVertices[i + 2];

		v.nx = k_cubeNormals[i + 0];
		v.ny = k_cubeNormals[i + 1];
		v.nz = k_cubeNormals[i + 2];

		cube.vertices.push_back(v);
	}

	for (uint16_t index : k_cubeIndices)
	{
		cube.indices.push_back(index);
	}

	return cube;
}

//TODO don't copy mesh on return
Mesh ModelLoader::LoadModel(std::string const& filePath)
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

		//Do nothing with texture index for now
	}

	std::vector<uint32_t> remap(indexCount);
	size_t totalVertices = meshopt_generateVertexRemap(remap.data(), nullptr, indexCount, vertices.data(), indexCount, sizeof(Vertex));

	Mesh mesh;
	mesh.vertices.resize(totalVertices);
	mesh.indices.resize(indexCount);

	meshopt_remapVertexBuffer(mesh.vertices.data(), vertices.data(), indexCount, sizeof(Vertex), remap.data());
	meshopt_remapIndexBuffer(mesh.indices.data(), nullptr, indexCount, remap.data());

	return mesh;
}
