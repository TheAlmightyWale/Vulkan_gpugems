#include "GfxStaticModelDrawer.h"
#include "GfxPipeline.h"
#include "GfxDescriptorManager.h"

void GfxStaticModelDrawer::DrawObjects(
	std::span<StaticModelPtr_t const> models,
	GfxPipeline const& pipeline,
	vk::CommandBuffer& secondaryCommandBuffer,
	GfxDescriptorManagerPtr_t const& descriptorManager)
{
	secondaryCommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.pipeline);

	secondaryCommandBuffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		*pipeline.layout,
		0,
		descriptorManager->GetDescriptors(),
		nullptr
		);

	for (uint32_t i = 0; auto const pModel : models)
	{
		secondaryCommandBuffer.bindVertexBuffers(0/*first binding*/, *pModel->GetVertexBuffer().m_buffer, { 0 } /*offset*/);
		secondaryCommandBuffer.bindIndexBuffer(*pModel->GetIndexBuffer().m_buffer, 0/*offset*/, vk::IndexType::eUint32);
		secondaryCommandBuffer.drawIndexed(pModel->GetIndexBuffer().m_dataSize / sizeof(uint32_t), 1/*instance count*/, 0, 0, i /*first instance*/);

		i++;
	}
}
