#pragma once
#include <span>
#include "GfxDevice.h"
#include "GfxFwdDecl.h"
#include "StaticModel.h"
#include "GfxDescriptorManager.h"

class GfxStaticModelDrawer
{
public:
	static void DrawObjects(
		std::span<StaticModelPtr_t const> models,
		GfxPipeline const& pipeline,
		vk::CommandBuffer& secondaryCommandBuffer,
		GfxDescriptorManagerPtr_t const& descriptorManager);
};