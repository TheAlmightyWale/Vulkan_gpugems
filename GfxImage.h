#pragma once

#include "GfxFwdDecl.h"

struct GfxImage {
	GfxImage() noexcept:
		image(nullptr),
		view(nullptr),
		memory(nullptr),
		sampler(nullptr)
	{}

	vk::raii::Image image;
	vk::raii::ImageView view;
	vk::raii::DeviceMemory memory;
	SamplerPtr_t sampler;
};