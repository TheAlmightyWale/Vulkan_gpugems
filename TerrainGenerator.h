#pragma once
#include <vector>
#include <memory>

#include "GfxFwdDecl.h"
#include "TerrainVertex.h"
#include "GfxDescriptorManager.h"


struct Mesh;
struct GfxPipeline;

struct Cell
{
	union {
		TerrainVertex TerrainVertexs[8];
		struct {
			//Front bottom left
			TerrainVertex fbl;
			//Front top left
			TerrainVertex ftl;
			//Front top right
			TerrainVertex ftr;
			//Front bottom right
			TerrainVertex fbr;
			//Back bottom left
			TerrainVertex bbl;
			//Back top left
			TerrainVertex btl;
			//Back top right
			TerrainVertex btr;
			//Back bottom right
			TerrainVertex bbr;
		};

	};
};

class TerrainGenerator
{
public:
	TerrainGenerator(GfxDevicePtr_t pDevice, vk::Viewport viewport, vk::Rect2D scissor, vk::RenderPass renderPass);

	//TODO Whats best practices for renderpasses? I'm assuming we want to minimize starting and ending passes
	vk::CommandBuffer Render(vk::RenderPass renderPass, vk::RenderPassBeginInfo passBeginInfo, GfxDevicePtr_t pDevice);

private:
	std::vector<Cell> m_grid;
	std::shared_ptr<GfxBuffer> m_pInputBuffer;
	std::shared_ptr<GfxBuffer> m_pOutputBuffer;

	//Common Render components
	std::unique_ptr<GfxPipeline> m_pPipeline;
	vk::raii::CommandPool m_graphicsCommandPool;
	vk::raii::CommandBuffer m_renderCommandBuffer;

	//Compute components
	std::unique_ptr<GfxPipeline> m_pComputePipline;
	GfxDescriptorManager m_computeDescriptors;
};