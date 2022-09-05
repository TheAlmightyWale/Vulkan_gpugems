#pragma once
#include "GfxApiInstance.h"
#include "GfxDevice.h"
#include "GfxSwapChain.h"
#include "GfxFrame.h"
#include "GfxImage.h"
#include "GfxPipeline.h"
#include "Mesh.h"
#include "GfxBuffer.h"
#include "GfxTextOverlay.h"
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

	GfxEngine(GfxEngine const&) = delete;
	GfxEngine(GfxEngine&&) = delete;
	GfxEngine& operator=(GfxEngine const&) = delete;
	GfxEngine& operator=(GfxEngine&&) = delete;


	void Render();

protected:
	GfxFrame& GetCurrentFrame();

private:

	vk::raii::ShaderModule LoadShaderModule(std::string const& filePath);

	WindowPtr_t m_pWindow;
	std::shared_ptr<GfxApiInstance> m_pInstance;
	std::shared_ptr<GfxDevice> m_pDevice;

	//TODO find a home
	vk::raii::SurfaceKHR m_surface;
	GfxSwapchain m_swapChain;
	vk::raii::RenderPass m_renderPass;
	std::vector<GfxFrame> m_frames;
	GfxImage m_depthBuffer;
	GfxPipeline m_pipeline;

	vk::raii::Queue m_graphicsQueue;

	//Functionality
	GfxTextOverlay m_textOverlay;

	Mesh m_cube;
	GfxBuffer m_modelVertexBuffer;
	GfxBuffer m_modelIndexBuffer;

	Camera m_camera;

	uint64_t m_numFramesRendered;

	//Perf timers
	vk::raii::QueryPool m_timingQueryPool;

	//TODO turn into hash map with hash based on sampler properties
	std::vector<vk::raii::Sampler> m_samplers;

};

