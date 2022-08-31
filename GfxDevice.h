#pragma once
#include "GfxFwdDecl.h"

class GfxDevice
{
public:
	GfxDevice(vk::raii::Instance const& pInstance,
		vk::PhysicalDeviceFeatures desiredFeatures,
		vk::PhysicalDeviceProperties desiredProperties,
		std::vector<char const*> enabledExtensions,
		std::vector<char const*> enabledLayers);
	~GfxDevice();

	vk::raii::CommandPool CreateGraphicsCommandPool();
	vk::raii::CommandBuffers CreateCommandBuffers(vk::CommandPool commandPool, uint32_t numBuffers);
	GfxSwapchain CreateSwapChain(vk::SurfaceKHR const& surface, uint32_t desiredSwapchainSize);
	GfxImage CreateDepthStencil(uint32_t width, uint32_t height, vk::Format depthFormat);
	vk::raii::Semaphore CreateVkSemaphore();
	vk::raii::Fence CreateFence();
	GfxBuffer CreateBuffer(size_t size, vk::BufferUsageFlags flags);
	GfxImage CreateImage(vk::ImageCreateInfo createInfo, vk::ImageAspectFlags aspect, vk::MemoryPropertyFlagBits desiredMemoryProperties);
	vk::raii::QueryPool CreateQueryPool(uint32_t queryCount);
	vk::raii::Sampler CreateTextureSampler();
	
	vk::Queue GetGraphicsQueue();
	vk::raii::Device const& GetDevice() const noexcept { return *m_pDevice.get(); }

	vk::PhysicalDeviceProperties GetProperties() const { return m_physcialDevice.getProperties(); }

private:
	vk::raii::PhysicalDevice m_physcialDevice;
	DevicePtr_t m_pDevice;
	uint32_t m_graphcsQueueFamilyIndex;
};

