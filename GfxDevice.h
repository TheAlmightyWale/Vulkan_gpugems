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
	vk::raii::CommandBuffers CreateCommandBuffers(vk::raii::CommandPool const& commandPool, uint32_t numBuffers);
	GfxSwapchain CreateSwapChain(vk::raii::SurfaceKHR const& surface);
	GfxImage CreateDepthStencil(uint32_t width, uint32_t height);
	GfxUniformBuffer CreateUniformBuffer(uint8_t* pDataSource, vk::DeviceSize numBytes, vk::SharingMode sharingMode);
	vk::raii::Semaphore CreateVkSemaphore();
	
	vk::Queue GetGraphicsQueue();
	vk::raii::Device const& GetDevice() const noexcept { return *m_pDevice.get(); }

private:
	vk::raii::PhysicalDevice m_physcialDevice;
	DevicePtr_t m_pDevice;
	uint32_t m_graphcsQueueFamilyIndex;
};

