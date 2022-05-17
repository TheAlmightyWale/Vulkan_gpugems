#pragma once
#include "GfxFwdDecl.h"

struct GfxFrame {
	GfxFrame(): frameBuffer(nullptr)
	{}

	vk::raii::Framebuffer frameBuffer;
};