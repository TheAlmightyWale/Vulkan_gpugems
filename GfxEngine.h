#pragma once
#include "GfxApiInstance.h"
#include "GfxDevice.h"
#include "GfxSwapChain.h"
#include "GfxFrame.h"
#include "GfxImage.h"
#include "GfxPipeline.h"
#include "Mesh.h"
#include "GfxBuffer.h"
#include "Camera.h"

std::string const k_engineName = "Vulkan?";
uint32_t const k_engineVersion = 1;
uint32_t const k_vulkanVersion = VK_API_VERSION_1_2;

class GfxEngine
{
public:
	GfxEngine(std::string const& applicationName, uint32_t appVersion, WindowPtr_t pWindow);
	~GfxEngine();

	void Render();

private:
	vk::raii::ShaderModule LoadShaderModule(std::string const& filePath);

	std::shared_ptr<GfxDevice> m_pDevice;
	std::shared_ptr<GfxApiInstance> m_pInstance;

	//TODO find a home
	vk::raii::RenderPass m_renderPass;
	std::vector<GfxFrame> m_frames;
	GfxSwapchain m_swapChain;
	GfxImage m_depthBuffer;
	vk::raii::SurfaceKHR m_surface;
	GfxPipeline m_pipeline;

	vk::raii::Queue m_graphicsQueue;

	//TODO should these be their own objects?
	vk::raii::CommandPool m_graphicsCommandPool;
	vk::raii::CommandBuffer m_graphicsCommandBuffer;

	//Rendering semaphores
	vk::raii::Semaphore m_aquireImageSemaphore;
	vk::raii::Semaphore m_readyToPresentSemaphore;

	Mesh m_cube;
	GfxBuffer m_cubeVertexBuffer;
	GfxBuffer m_cubeIndexBuffer;

	Camera m_camera;

};

