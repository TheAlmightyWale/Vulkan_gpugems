#pragma once
#include "GfxFwdDecl.h"

struct TerrainVertex {
	float x, y, z;

	static VertexDescription GetDescription()
	{
		std::vector < vk::VertexInputBindingDescription> bindings =
		{
			vk::VertexInputBindingDescription(0, sizeof(TerrainVertex))
		};

		std::vector<vk::VertexInputAttributeDescription> attributes =
		{
			vk::VertexInputAttributeDescription(0 /*location*/, 0 /*binding*/, vk::Format::eR32G32B32Sfloat, 0/*offset*/),
		};

		return VertexDescription{
			.attributes = attributes,
			.bindings = bindings
		};
	}

	TerrainVertex operator+(TerrainVertex const& b) const
	{
		return { x + b.x, y + b.y, z + b.z };
	}

	TerrainVertex operator/(float const& denom) const
	{
		return { x / denom, y / denom, z / denom };
	}
};