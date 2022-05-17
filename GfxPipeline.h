#pragma once
#include "GfxFwdDecl.h"

struct GfxPipeline {
	GfxPipeline() noexcept:
		layout(nullptr),
		pipeline(nullptr),
		descriptorPool(nullptr),
		descriptorSet(nullptr) {}

	vk::raii::PipelineLayout layout;
	vk::raii::Pipeline pipeline;

	vk::raii::DescriptorPool descriptorPool;
	vk::raii::DescriptorSet descriptorSet;
};