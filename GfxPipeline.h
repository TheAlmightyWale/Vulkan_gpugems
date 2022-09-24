#pragma once
#include "GfxFwdDecl.h"

struct GfxPipeline {
	GfxPipeline() noexcept:
		layout(nullptr),
		pipeline(nullptr)
	{}

	vk::raii::PipelineLayout layout;
	vk::raii::Pipeline pipeline;
};