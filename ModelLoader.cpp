#include "ModelLoader.h"
#include "lib/objparser/objparser.h"
#include "Exceptions.h"
//TODO turn mesh optimizer into a library
#include "lib/meshoptimizer/src/meshoptimizer.h"

//class ObjRaii
//{
//public:
//	ObjRaii(std::string const& filePath)
//	{
//		m_pMesh = fast_obj_read(filePath.c_str());
//		if (m_pMesh == nullptr)
//		{
//			throw InitializationException("Failed to load mesh from: " + filePath);
//		}
//	}
//
//	ObjRaii(ObjRaii const&) = delete;
//	ObjRaii(ObjRaii&&) = delete;
//	ObjRaii& operator=(ObjRaii const&) = delete;
//	ObjRaii& operator=(ObjRaii&&) = delete;
//
//	~ObjRaii()
//	{
//		if (m_pMesh)
//			fast_obj_destroy(m_pMesh);
//	}
//
//	fastObjMesh* GetMesh() const
//	{
//		return m_pMesh;
//	}
//
//private:
//	fastObjMesh* m_pMesh;
//};

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

	//TODO remove leftover fast_obj stuff

	//auto* const pMesh = parsedObj.GetMesh();

	////Create mesh object then optimize it
	//size_t totalIndices = 0;
	//for (uint32_t i = 0; i < pMesh->face_count; ++i)
	//{
	//	//TODO why 2?
	//	totalIndices += 3 * (pMesh->face_vertices[i] - 2);
	//}

	//std::vector<Vertex> vertices(totalIndices);
	//std::vector<uint32_t> indices(totalIndices);

	//size_t vertexOffset = 0;
	//size_t indexOffset = 0;

	//for (uint32_t faceIndex = 0; faceIndex < pMesh->face_count; ++faceIndex)
	//{
	//	for (uint32_t faceVertexIndex = 0; faceVertexIndex < pMesh->face_vertices[faceIndex]; ++faceVertexIndex)
	//	{
	//		fastObjIndex index = pMesh->indices[indexOffset + faceVertexIndex];
	//		Vertex vert =
	//		{
	//			pMesh->positions[index.p * 3 + 0],
	//			pMesh->positions[index.p * 3 + 1],
	//			pMesh->positions[index.p * 3 + 2],
	//			pMesh->normals[index.n * 3 + 0],
	//			pMesh->normals[index.n * 3 + 1],
	//			pMesh->normals[index.n * 3 + 2]
	//		};

	//		vertices[vertexOffset] = vert;
	//		vertexOffset++;

	//		if (faceVertexIndex >= 3)
	//		{
	//			//Triangulate
	//			vertices[vertexOffset + 0] = vertices[vertexOffset - 3];
	//			vertices[vertexOffset + 1] = vertices[vertexOffset - 1];
	//			vertexOffset += 2;
	//		}
	//		indices[indexOffset + faceVertexIndex] = index.p;
	//	}
	//	indexOffset += pMesh->face_vertices[faceIndex];
	//}

	////Run loaded vertices through optimizer
	//std::vector<uint32_t> remap(totalIndices);
	//size_t totalVertices = meshopt_generateVertexRemap(remap.data(), indices.data(), totalIndices, vertices.data(), totalIndices, sizeof(Vertex));

	//Mesh mesh;
	//mesh.vertices.resize(totalVertices);
	//mesh.indices.resize(totalIndices);

	//meshopt_remapVertexBuffer(mesh.vertices.data(), vertices.data(), totalIndices, sizeof(Vertex), remap.data());
	//meshopt_remapIndexBuffer(mesh.indices.data(), indices.data(), totalIndices, remap.data());

	////TODO optimize vertex caching and fetching with mesh optimizer?

	/*return mesh;*/
}
