#pragma once

#include "GfxFwdDecl.h"

struct GfxImage {
	GfxImage():
		image(nullptr),
		view(nullptr),
		memory(nullptr),
		sampler()
	{}

	vk::raii::Image image;
	vk::raii::ImageView view;
	vk::raii::DeviceMemory memory;
	SamplerPtr_t sampler;
};