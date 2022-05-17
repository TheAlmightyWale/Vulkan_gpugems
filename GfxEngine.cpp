#include "GfxEngine.h"
#include "GfxFrame.h"
#include "GfxDevice.h"
#include "GfxImage.h"
#include "GfxUniformBuffer.h"
#include "GfxPipeline.h"
#include "GfxPipelineBuilder.h"
#include "GfxSwapchain.h"
#include "Logger.h"
#include "Exceptions.h"

#include <fstream>

//Temp for uniform buffer testing, move to camera class 
#include "Math.h"

//TODO wrap extensions and layers into configurable features?
std::vector<const char*> const k_deviceExtensions{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
};

std::vector<const char*> const k_deviceLayers{
	//Empty for now
};

GfxEngine::GfxEngine(std::string const& applicationName, uint32_t appVersion, WindowPtr_t pWindow)
	: m_pInstance(nullptr)
	, m_pDevice(nullptr)
	, m_renderPass(nullptr)
	, m_surface(nullptr)
	, m_graphicsQueue(nullptr)
	, m_graphicsCommandPool(nullptr)
	, m_graphicsCommandBuffer(nullptr)
	, m_aquireImageSemaphore(nullptr)
	, m_readyToPresentSemaphore(nullptr)
{

	m_pInstance = std::make_shared<GfxApiInstance>(applicationName, appVersion, k_engineName, k_engineVersion, k_vulkanVersion);

	//Create device
	vk::PhysicalDeviceFeatures desiredFeatures;
	//desiredFeatures.geometryShader = VK_TRUE;
	vk::PhysicalDeviceProperties desiredProperties;
	//desiredProperties.deviceType = vk::PhysicalDeviceType::eDiscreteGpu;
	desiredProperties.apiVersion = k_vulkanVersion;

	m_pDevice = std::make_shared<GfxDevice>(m_pInstance->GetInstance(), desiredFeatures, desiredProperties, k_deviceExtensions, k_deviceLayers);

	m_graphicsCommandPool = m_pDevice->CreateGraphicsCommandPool();
	vk::CommandBufferAllocateInfo allocateInfo(*m_graphicsCommandPool, vk::CommandBufferLevel::ePrimary, 1/*Command buffer count*/);
	m_graphicsCommandBuffer = std::move(m_pDevice->CreateCommandBuffers(m_graphicsCommandPool, 1/*num buffers*/).front());

	//Load shaders
	vk::raii::ShaderModule fragmentShader = LoadShaderModule("triangle.frag.spv");
	vk::raii::ShaderModule vertexShader = LoadShaderModule("triangle.vert.spv");

	VkSurfaceKHR _surface;
	glfwCreateWindowSurface(*m_pInstance->GetInstance(), pWindow->Get(), nullptr, &_surface);
	m_surface = std::move(vk::raii::SurfaceKHR(m_pInstance->GetInstance(), _surface));
	m_swapChain = m_pDevice->CreateSwapChain(m_surface);

	auto [width, height] = pWindow->GetWindowSize();
	m_depthBuffer = m_pDevice->CreateDepthStencil(width, height);

	//TODO view/projection should be held in camera class
	glm::mat4x4 view = glm::lookAt(glm::vec3{ -5.0f, 3.0f, -10.0f }/*eye*/, glm::vec3{ 0.0f,0.0f,0.0f }/*target*/, glm::vec3{ 0.0f, -1.0f, 0.0f }/*up*/); //TODO double check up direction
	glm::mat4x4 projection = glm::perspective(glm::radians(45.0f)/*fov*/, 1.0f/*aspect*/, 0.1f/*near*/, 100.0f/*far*/);
	//Transforms open-gl clip format to vulkan format. vulkan has an inverted Y axis and a clipping z plane from 0 to 1 rather than -1 to 1
	glm::mat4x4 clipTransform = glm::mat4x4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f);

	glm::mat4x4 vpc = clipTransform * projection * view;
	GfxUniformBuffer viewProjectionTransform = m_pDevice->CreateUniformBuffer(
		reinterpret_cast<uint8_t*>(&vpc),
		sizeof(vpc),
		vk::SharingMode::eExclusive);

	//Create attachments
	//Attachments describe what image formats/target formats we want write to / read from
	vk::Format renderSurfaceFormat = vk::Format::eB8G8R8A8Unorm;
	vk::Format depthSurfaceFormat = vk::Format::eD16Unorm;

	std::array<vk::AttachmentDescription, 1> renderPassAttachments;
	renderPassAttachments[0] = vk::AttachmentDescription(
		{} /*flags*/,
		renderSurfaceFormat,
		vk::SampleCountFlagBits::e1,
		/*main surface ops*/
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eStore,
		/*stencil ops*/
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare,
		/*layout transition*/
		vk::ImageLayout::eUndefined, //initial
		vk::ImageLayout::ePresentSrcKHR //final
	);

	//renderPassAttachments[1] = vk::AttachmentDescription(
	//	{} /*flags*/,
	//	depthSurfaceFormat,
	//	vk::SampleCountFlagBits::e1,
	//	vk::AttachmentLoadOp::eClear,
	//	vk::AttachmentStoreOp::eDontCare,
	//	vk::AttachmentLoadOp::eDontCare,
	//	vk::AttachmentStoreOp::eDontCare,
	//	vk::ImageLayout::eUndefined,
	//	vk::ImageLayout::eDepthStencilAttachmentOptimal
	//);

	//Add to renderpass
	m_renderPass = GfxPipelineBuilder::CreateRenderPass(
		m_pDevice->GetDevice(),
		viewProjectionTransform,
		renderSurfaceFormat,
		depthSurfaceFormat,
		renderPassAttachments);

	//Now we have a renderpass defined we need to connect actual image resources to it
	std::vector<vk::ImageView> views = m_swapChain.GetImageViews();
	m_frames = std::vector<GfxFrame>(views.size());

	for (uint32_t i = 0; i < views.size(); ++i)
	{
		vk::FramebufferCreateInfo frameBufferCreateInfo(
			{},
			*m_renderPass,
			views[i],
			width,
			height,
			1
		);

		m_frames[i].frameBuffer = vk::raii::Framebuffer(m_pDevice->GetDevice(), frameBufferCreateInfo);
	}

	m_pipeline.layout = GfxPipelineBuilder::CreatePipelineLayout(m_pDevice->GetDevice());

	GfxPipelineBuilder builder;
	builder._shaderStages.push_back(
		GfxPipelineBuilder::CreateShaderStageInfo(vk::ShaderStageFlagBits::eVertex, *vertexShader)
	);
	builder._shaderStages.push_back(
		GfxPipelineBuilder::CreateShaderStageInfo(vk::ShaderStageFlagBits::eFragment, *fragmentShader)
	);

	builder._vertexInputInfo = GfxPipelineBuilder::CreateVertexInputStateInfo();
	builder._inputAssembly = GfxPipelineBuilder::CreateInputAssemblyInfo(vk::PrimitiveTopology::eTriangleList);
	builder._viewport.setX(0.0f);
	builder._viewport.setY((float)height);
	builder._viewport.setWidth((float)width);
	builder._viewport.setHeight(-(float)height);
	builder._viewport.setMinDepth(0.0f);
	builder._viewport.setMaxDepth(1.0f);

	builder._scissor.setOffset({ 0, 0 });
	builder._scissor.setExtent({ width, height });

	builder._rasterizer = GfxPipelineBuilder::CreateRasterizationStateInfo(vk::PolygonMode::eFill);
	builder._multisampling = GfxPipelineBuilder::CreateMultisampleStateInfo();
	builder._colorBlendAttachment = GfxPipelineBuilder::CreateColorBlendAttachmentState();
	builder._pipelineLayout = *m_pipeline.layout;

	m_pipeline.pipeline = builder.BuildPipeline(m_pDevice->GetDevice(), *m_renderPass);

	m_aquireImageSemaphore = m_pDevice->CreateVkSemaphore();
	m_readyToPresentSemaphore = m_pDevice->CreateVkSemaphore();
}

GfxEngine::~GfxEngine()
{
}

void GfxEngine::Render()
{
	uint64_t const k_aquireTimeout_ns = 100000000; //0.1 seconds
	auto [result, imageIndex] = m_swapChain.m_swapchain.acquireNextImage(k_aquireTimeout_ns, *m_aquireImageSemaphore);
	//TODO: check and handle failed aquisition

	m_graphicsCommandPool.reset();
	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	m_graphicsCommandBuffer.begin(beginInfo);


	vk::ClearColorValue const k_clearColor(std::array<float, 4>{48.0f / 2550.f, 10.0f / 255.0f, 36.0f / 255.0f, 1.0f});
	vk::ClearValue const clearValue{ k_clearColor };
	vk::Rect2D const renderArea({ 0,0 }, m_swapChain.m_extent);
	vk::RenderPassBeginInfo passBeginInfo(*m_renderPass, *m_frames[imageIndex].frameBuffer, renderArea, clearValue);

	m_graphicsCommandBuffer.beginRenderPass(passBeginInfo, vk::SubpassContents::eInline);
	m_graphicsCommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline.pipeline);
	m_graphicsCommandBuffer.draw(3, 1, 0, 0);
	m_graphicsCommandBuffer.endRenderPass();

	m_graphicsCommandBuffer.end();

	vk::PipelineStageFlags submitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::Queue const queue = m_pDevice->GetGraphicsQueue();
	vk::SubmitInfo renderSubmitInfo(*m_aquireImageSemaphore, submitStageMask, *m_graphicsCommandBuffer, *m_readyToPresentSemaphore);
	queue.submit(renderSubmitInfo);

	vk::PresentInfoKHR presentInfo(*m_readyToPresentSemaphore, *m_swapChain.m_swapchain, imageIndex);
	queue.presentKHR(presentInfo);//TODO handle different Success results

	//TODO wait on a render finished semaphore properly
	m_pDevice->GetDevice().waitIdle();
}

vk::raii::ShaderModule GfxEngine::LoadShaderModule(std::string const& filePath)
{
	//TODO RAII files?
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw InvalidStateException("Failed to load shader module at: " + filePath);
	}

	//Since cursor starts at end we can get file size by reading it's position
	size_t fileSize = (size_t)file.tellg();

	//spriv expects buffers to be measure in 32bits
	std::vector<uint32_t> buffer(fileSize / (sizeof(uint32_t)));

	file.seekg(0);
	file.read((char*)buffer.data(), fileSize);
	file.close();

	//size is expected to be in 32bit size
	vk::ShaderModuleCreateInfo createInfo({}, buffer.size() * sizeof(uint32_t), buffer.data());
	vk::raii::ShaderModule shaderModule(m_pDevice->GetDevice(), createInfo);

	SPDLOG_INFO("Loaded shader found at: " + filePath);

	return std::move(shaderModule);
}
