#include "TerrainGenerator.h"
#include "GfxPipeline.h"
#include "GfxPipelineBuilder.h"
#include "GfxDevice.h"
#include "GfxBuffer.h"
#include "ShaderLoader.h"


//For mat4 size
#include "Math.h"

constexpr uint32_t k_valuesGenerated = 256;
constexpr uint32_t k_densityInputBindingId = 0;
constexpr uint32_t k_densityOutputBindingId = 1;

TerrainGenerator::TerrainGenerator(GfxDevicePtr_t pDevice, vk::Viewport viewport, vk::Rect2D scissor, vk::RenderPass renderPass)
	: m_pPipeline(std::make_unique<GfxPipeline>())
	, m_pComputePipline(std::make_unique<GfxPipeline>())
	, m_computeDescriptors(pDevice)
	, m_grid()
	, m_pInputBuffer(nullptr)
	, m_pOutputBuffer(nullptr)
	, m_graphicsCommandPool(nullptr)
	, m_renderCommandBuffer(nullptr)
{
	//Set up compute pipeline
	m_computeDescriptors.AddBinding(
		k_densityInputBindingId,
		vk::ShaderStageFlagBits::eCompute,
		DataUsageFrequency::ePerFrame,
		vk::DescriptorType::eStorageBuffer);

	m_computeDescriptors.AddBinding(
		k_densityOutputBindingId,
		vk::ShaderStageFlagBits::eCompute,
		DataUsageFrequency::ePerFrame,
		vk::DescriptorType::eStorageBuffer
	);

	vk::DescriptorSetLayout densityLayout = m_computeDescriptors.GetLayout(DataUsageFrequency::ePerFrame);

	std::vector<vk::DescriptorSetLayout> densityComputeInputs{
		densityLayout
	};
	m_pComputePipline->layout = GfxPipelineBuilder::CreatePipelineLayout(pDevice->GetDevice(), nullptr, densityComputeInputs);

	vk::raii::ShaderModule densityComputeShader = ShaderLoader::LoadModule("densityGenerator.comp.spv", pDevice);

	vk::PipelineShaderStageCreateInfo computePipelineStageCreateInfo =
		GfxPipelineBuilder::CreateShaderStageInfo(vk::ShaderStageFlagBits::eCompute, *densityComputeShader);

	vk::ComputePipelineCreateInfo computeCreateInfo(
		{} /* create flags*/,
		computePipelineStageCreateInfo,
		* m_pComputePipline->layout
	);
	m_pComputePipline->pipeline = pDevice->GetDevice().createComputePipeline(nullptr, computeCreateInfo, nullptr);

	//Set up graphics pipeline
	//m_pPipeline->layout = GfxPipelineBuilder::CreatePipelineLayout(pDevice->GetDevice(), nullptr, nullptr);

	//vk::raii::ShaderModule vertexShader = ShaderLoader::LoadModule("gooch.vert.spv", pDevice);
	//vk::raii::ShaderModule fragShader = ShaderLoader::LoadModule("gooch.frag.spv", pDevice);

	//GfxPipelineBuilder builder;
	//builder._shaderStages.push_back(GfxPipelineBuilder::CreateShaderStageInfo(vk::ShaderStageFlagBits::eVertex, *vertexShader));
	//builder._shaderStages.push_back(GfxPipelineBuilder::CreateShaderStageInfo(vk::ShaderStageFlagBits::eFragment, *fragShader));
	//builder._inputAssembly = GfxPipelineBuilder::CreateInputAssemblyInfo(vk::PrimitiveTopology::eTriangleList);
	//builder._viewport = viewport;
	//builder._scissor = scissor;
	//builder._rasterizer = GfxPipelineBuilder::CreateRasterizationStateInfo(vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone);
	//builder._depthStencil = GfxPipelineBuilder::CreateDepthStencilStateInfo(VK_TRUE, VK_TRUE, vk::CompareOp::eLess);
	//builder._multisampling = GfxPipelineBuilder::CreateMultisampleStateInfo();
	//builder._colorBlendAttachment = GfxPipelineBuilder::CreateColorBlendAttachmentState();
	//builder._pipelineLayout = *m_pPipeline->layout;
	//builder._vertexDescription = TerrainVertex::GetDescription();

	//m_pPipeline->pipeline = builder.BuildPipeline(pDevice->GetDevice(), renderPass);

	//Set up commands
	m_graphicsCommandPool = pDevice->CreateGraphicsCommandPool();
	m_renderCommandBuffer = std::move(pDevice->CreateCommandBuffers(*m_graphicsCommandPool, 1).front());

	////Set up terrain voxels
	////For now we just create a grid at 0,0,0 that is 10 x 10 x 10
	//constexpr uint32_t k_gridSize = 10;
	//constexpr float k_cellSize = 5.0f;
	//m_grid.reserve(k_gridSize * k_gridSize * k_gridSize);
	//for (uint32_t height = 0; height < k_gridSize; ++height) {
	//	for (uint32_t width = 0; width < k_gridSize; ++width) {
	//		for (uint32_t depth = 0; depth < k_gridSize; ++depth) {
	//			Cell cell;
	//			cell.fbl = { .x = width * k_cellSize, .y = height * k_cellSize, .z = depth * k_cellSize };
	//			cell.bbl = { .x = width * k_cellSize, .y = height * k_cellSize, .z = (depth + 1) * k_cellSize };
	//			cell.bbr = { .x = (width + 1) * k_cellSize, .y = height * k_cellSize, .z = (depth + 1) * k_cellSize };
	//			cell.fbr = { .x = (width + 1) * k_cellSize, .y = height * k_cellSize, .z = depth * k_cellSize };

	//			cell.ftl = { .x = width * k_cellSize, .y = (height + 1) * k_cellSize, .z = depth * k_cellSize };
	//			cell.btl = { .x = width * k_cellSize, .y = (height + 1) * k_cellSize, .z = (depth + 1) * k_cellSize };
	//			cell.btr = { .x = (width + 1) * k_cellSize, .y = (height + 1) * k_cellSize, .z = (depth + 1) * k_cellSize };
	//			cell.ftr = { .x = (width + 1) * k_cellSize, .y = (height + 1) * k_cellSize, .z = depth * k_cellSize };

	//			m_grid.emplace_back(cell);
	//		}
	//	}
	//}

	//Upload vertices to GPU
	size_t const inputBufferSize = sizeof(glm::mat4); //TODO get camera transform and noise texture to upload
	m_pInputBuffer = std::make_shared<GfxBuffer>(pDevice->CreateBuffer(inputBufferSize, vk::BufferUsageFlagBits::eStorageBuffer));
	glm::mat4 ident = glm::identity<glm::mat4>();
	m_pInputBuffer->CopyToBuffer(&ident, inputBufferSize, 0);

	size_t const outputBufferSize = k_valuesGenerated * sizeof(float);
	m_pOutputBuffer = std::make_shared<GfxBuffer>(pDevice->CreateBuffer(outputBufferSize, vk::BufferUsageFlagBits::eStorageBuffer));
	std::vector<float> empty(k_valuesGenerated, 0.0f);
	m_pOutputBuffer->CopyToBuffer(empty.data(), outputBufferSize, 0);
	vk::WriteDescriptorSet writeDescriptor = m_computeDescriptors.GetWriteDescriptor(DataUsageFrequency::ePerFrame, k_densityOutputBindingId);
	pDevice->UploadBufferData(outputBufferSize, 0, *m_pOutputBuffer->m_buffer, writeDescriptor);
}

vk::CommandBuffer TerrainGenerator::Render(vk::RenderPass renderPass, vk::RenderPassBeginInfo passBeginInfo, GfxDevicePtr_t pDevice)
{
	m_graphicsCommandPool.reset();
	//Terrain generation algorithim is as follows

	//Upload noise texture once at initialization
	//Upload marching cube configurations once at initialization

	//Wait for previous frame to finish
	//Update camera data per frame
	vk::WriteDescriptorSet writeDescriptor = m_computeDescriptors.GetWriteDescriptor(DataUsageFrequency::ePerFrame, k_densityInputBindingId);
	pDevice->UploadBufferData(sizeof(glm::mat4), 0, *m_pInputBuffer->m_buffer, writeDescriptor);

	//Run compute shader to generate grid values

	vk::CommandBufferBeginInfo const beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	m_renderCommandBuffer.begin(beginInfo);

	m_renderCommandBuffer.bindDescriptorSets(
		vk::PipelineBindPoint::eCompute,
		*m_pComputePipline->layout,
		0,
		m_computeDescriptors.GetDescriptor(DataUsageFrequency::ePerFrame),
		nullptr
	);

	m_renderCommandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *m_pComputePipline->pipeline);
	m_renderCommandBuffer.dispatch(k_valuesGenerated, 1, 1);

	//Wait on compute shader to complete
	//TODO barrier

	m_renderCommandBuffer.beginRenderPass(passBeginInfo, vk::SubpassContents::eInline);

	//Feed grid values to mesh shader
	//Use mesh shader to generate terrain

	//Draw visible terrain with fragment shader

	m_renderCommandBuffer.endRenderPass();

	m_renderCommandBuffer.end();
	return *m_renderCommandBuffer;


}
