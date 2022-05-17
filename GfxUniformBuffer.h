#pragma once
#include "GfxFwdDecl.h"

struct GfxUniformBuffer {
	GfxUniformBuffer()
		: buffer(nullptr)
		, memory(nullptr)
	{}

	vk::raii::Buffer buffer;
	vk::raii::DeviceMemory memory;
};