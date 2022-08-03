#pragma once
#include "GfxFwdDecl.h"

struct GfxFrame {
	GfxFrame():
		frameBuffer(nullptr)
		, aquireImageSemaphore(nullptr)
		, readyToPresentSemaphore(nullptr)
		, renderCompleteFence(nullptr)
		, commandPool(nullptr)
		, commandBuffer(nullptr)
	{}

	vk::raii::Framebuffer frameBuffer;
	//Rendering semaphores
	vk::raii::Semaphore aquireImageSemaphore;
	vk::raii::Semaphore readyToPresentSemaphore;
	vk::raii::Fence renderCompleteFence;

	vk::raii::CommandPool commandPool;
	vk::raii::CommandBuffer commandBuffer;
};