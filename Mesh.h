#pragma once
#include <vector>
#include "GfxFwdDecl.h"
#include "GfxBuffer.h"

struct Vertex
{
	float vx, vy, vz;
	float nx, ny, nz;

	static VertexDescription GetDescription()
	{
		std::vector < vk::VertexInputBindingDescription> bindings =
		{
			vk::VertexInputBindingDescription(0, sizeof(Vertex)) //Vertex should be 4 * 6 = 24bytes
		};
		
		std::vector<vk::VertexInputAttributeDescription> attributes =
		{
			vk::VertexInputAttributeDescription(0 /*location*/, 0 /*binding*/, vk::Format::eR32G32B32Sfloat, 0/*offset*/),
			vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, 12 /*3 floats from position*/)
		};

		return VertexDescription{
			.attributes = attributes,
			.bindings = bindings
		};
	}
};

struct TextVertex
{
	float vx, vy;
	float u, v;

	static VertexDescription GetDescription()
	{
		std::vector < vk::VertexInputBindingDescription> bindings =
		{
			vk::VertexInputBindingDescription(0, sizeof(TextVertex)) // 4 floats = 16 bytes
		};

		std::vector<vk::VertexInputAttributeDescription> attributes =
		{
			vk::VertexInputAttributeDescription(0 /*location*/, 0 /*binding*/, vk::Format::eR32G32Sfloat, 0/*offset*/), //Position
			vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32Sfloat, 8 /*2 floats from position*/)			// UV
		};

		return VertexDescription{
			.attributes = attributes,
			.bindings = bindings
		};
	}
};


struct Mesh
{
	GfxBuffer vertexBuffer;
	GfxBuffer indexBuffer;
};
using MeshPtr_t = std::shared_ptr<Mesh>;


using MeshPool = std::unordered_map<std::string, MeshPtr_t>;
using MeshPoolPtr_t = std::shared_ptr<MeshPool>;