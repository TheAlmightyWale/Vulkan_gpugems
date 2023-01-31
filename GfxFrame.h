#pragma once
#include "GfxFwdDecl.h"

struct GfxFrame {
	GfxFrame():
		frameBuffer(nullptr)
		, aquireImageSemaphore(nullptr)
		, readyToPresentSemaphore(nullptr)
		, renderCompleteFence(nullptr)
		, commandPool(nullptr)
		, commandBuffers(nullptr)
		, secondaryCommandBuffers(nullptr)
	{}

	vk::raii::Framebuffer frameBuffer;
	//Rendering semaphores
	vk::raii::Semaphore aquireImageSemaphore;
	vk::raii::Semaphore readyToPresentSemaphore;
	vk::raii::Fence renderCompleteFence;

	vk::raii::CommandPool commandPool;
	vk::raii::CommandBuffers commandBuffers;
	vk::raii::CommandBuffers secondaryCommandBuffers;
};