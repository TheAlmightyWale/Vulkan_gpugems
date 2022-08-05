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
uint32_t const k_numFramesBuffered = 2; //Double buffering
uint32_t const k_queryPoolCount = 64;

class GfxEngine
{
public:
	GfxEngine(std::string const& applicationName, uint32_t appVersion, WindowPtr_t pWindow);
	~GfxEngine();

	void Render();

protected:
	GfxFrame& GetCurrentFrame();

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

	Mesh m_cube;
	GfxBuffer m_cubeVertexBuffer;
	GfxBuffer m_cubeIndexBuffer;

	Camera m_camera;

	uint64_t m_numFramesRendered;

	WindowPtr_t m_pWindow;

	//Perf timers
	vk::raii::QueryPool m_timingQueryPool;

};

