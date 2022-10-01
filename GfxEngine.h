#pragma once
#include "GfxApiInstance.h"
#include "GfxDevice.h"
#include "GfxSwapChain.h"
#include "GfxFrame.h"
#include "GfxImage.h"
#include "GfxPipeline.h"
#include "StaticModel.h"
#include "GfxBuffer.h"
#include "GfxTextOverlay.h"
#include "GfxDescriptorManager.h"
#include "Camera.h"

//TODO move out object managment to scene loading
#include "ObjectProcessor.h"

std::string const k_engineName = "Vulkan?";
uint32_t const k_engineVersion = 1;
uint32_t const k_vulkanVersion = VK_API_VERSION_1_2;
uint32_t const k_numFramesBuffered = 2; //Double buffering
uint32_t const k_queryPoolCount = 64;

class GfxEngine
{
public:
	GfxEngine(std::string const& applicationName, uint32_t appVersion, WindowPtr_t pWindow, std::shared_ptr<ObjectProcessor> objectProcessor);
	~GfxEngine();

	GfxEngine(GfxEngine const&) = delete;
	GfxEngine(GfxEngine&&) = delete;
	GfxEngine& operator=(GfxEngine const&) = delete;
	GfxEngine& operator=(GfxEngine&&) = delete;


	void Render();

protected:
	GfxFrame& GetCurrentFrame();
	size_t PrepObjectDataForUpload();

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
	//Text
	GfxTextOverlay m_textOverlay;

	//Meshes
	MeshPoolPtr_t m_pMeshPool;
	std::vector<StaticModelPtr_t> m_models;

	uint64_t m_numFramesRendered;

	//TODO move out scene info
	std::shared_ptr<Camera> m_pCamera;
	GfxDescriptorManagerPtr_t m_pDescriptorManager;
	GfxBuffer m_lightBuffer;
	GfxBuffer m_objectDataBuffer;
	std::shared_ptr<ObjectProcessor> m_pObjectProcessor;

	//Perf timers
	vk::raii::QueryPool m_timingQueryPool;

	//TODO turn into hash map with hash based on sampler properties
	std::vector<vk::raii::Sampler> m_samplers;

};

