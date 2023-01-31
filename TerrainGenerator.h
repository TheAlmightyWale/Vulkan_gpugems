#pragma once
#include <vector>
#include <memory>

#include "GfxFwdDecl.h"
#include "TerrainVertex.h"
#include "GfxDescriptorManager.h"


struct Mesh;
struct GfxPipeline;
class Camera;

struct Edge
{
	uint8_t a;
	uint8_t b;
};

//Wind from bottom left point on plane clockwise. Then top left point on plane clockwise then middle (vertical) edges of cube
static Edge k_EdgeToVertexLookupTable[12] =
{
	{0,1},
	{1,2},
	{2,3},
	{3,0},
	{4,5},
	{5,6},
	{6,7},
	{7,4},
	{4,0},
	{5,1},
	{6,2},
	{7,3}
};

struct Cell
{
	union {
		TerrainVertex TerrainVertices[8];
		struct {
			//Back bottom left
			TerrainVertex bbl;
			//Back bottom right
			TerrainVertex bbr;
			//Front bottom right
			TerrainVertex fbr;
			//Front bottom left
			TerrainVertex fbl;
			//Back top left
			TerrainVertex btl;
			//Back top right
			TerrainVertex btr;
			//Front top right
			TerrainVertex ftr;
			//Front top left
			TerrainVertex ftl;
		};

	};
};

class TerrainGenerator
{
public:
	TerrainGenerator(GfxDevicePtr_t pDevice, vk::Viewport viewport, vk::Rect2D scissor, vk::RenderPass renderPass);

	vk::CommandBuffer Render(GfxDevicePtr_t pDevice);
	vk::CommandBuffer RenderTerrain(vk::CommandBufferInheritanceInfo const* pInheritanceInfo, GfxDevicePtr_t pDevice, Camera const& camera);

	void GenerateVertexBuffer(std::vector<float> const& lookUpIndices);
	std::vector<float> GetDensityOutput();

	bool ReadyToRender();

private:
	std::vector<Cell> m_grid;
	std::shared_ptr<GfxBuffer> m_pInputBuffer;
	std::shared_ptr<GfxBuffer> m_pOutputBuffer;

	//Common Render components
	std::unique_ptr<GfxPipeline> m_pPipeline;
	vk::raii::CommandPool m_graphicsCommandPool;
	vk::raii::CommandBuffer m_renderCommandBuffer;
	std::vector<TerrainVertex> m_vertices;
	//Temp
	std::shared_ptr<GfxBuffer> m_pVertexBuffer;


	//Compute components
	std::unique_ptr<GfxPipeline> m_pComputePipline;
	GfxDescriptorManager m_computeDescriptors;
	vk::raii::CommandBuffer m_generateCommandBuffer;
};