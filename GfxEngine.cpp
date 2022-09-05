#include "GfxEngine.h"
#include "GfxFrame.h"
#include "GfxDevice.h"
#include "GfxImage.h"
#include "GfxPipeline.h"
#include "GfxPipelineBuilder.h"
#include "GfxSwapchain.h"
#include "Logger.h"
#include "Exceptions.h"

#include <fstream>

//TODO Temp for uniform buffer testing, move to camera class 
#include "Math.h"

//TODO move somewhere else also
#include "Cube.h"
#include "ModelLoader.h"

Mesh LoadCube()
{
	Mesh cube;
	for (uint32_t i = 0; i < k_cubeVertexValuesCount; i+= 3)
	{
		Vertex v{};
		v.vx = k_cubeVertices[i + 0];
		v.vy = k_cubeVertices[i + 1];
		v.vz = k_cubeVertices[i + 2];

		v.nx = k_cubeNormals[i + 0];
		v.ny = k_cubeNormals[i + 1];
		v.nz = k_cubeNormals[i + 2];

		cube.vertices.push_back(v);
	}

	for (uint16_t index : k_cubeIndices)
	{
		cube.indices.push_back(index);
	}

	return cube;
}
//TODO wrap extensions and layers into configurable features?
std::vector<const char*> const k_deviceExtensions{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
};

std::vector<const char*> const k_deviceLayers{
	//Deprecated, but might be needed for backwards compatibility if we ever want it
};

GfxEngine::GfxEngine(std::string const& applicationName, uint32_t appVersion, WindowPtr_t pWindow)
	: m_pInstance(nullptr)
	, m_pWindow(pWindow)
	, m_pDevice(nullptr)
	, m_surface(nullptr)
	, m_swapChain()
	, m_renderPass(nullptr)
	, m_frames()
	, m_depthBuffer()
	, m_pipeline()
	, m_graphicsQueue(nullptr)
	, m_textOverlay()
	, m_cube()
	, m_modelVertexBuffer()
	, m_modelIndexBuffer()
	, m_camera(pWindow->GetWindowWidth(), pWindow->GetWindowHeight())
	, m_numFramesRendered(0)
	, m_timingQueryPool(nullptr)
	, m_samplers()
{

	m_pInstance = std::make_shared<GfxApiInstance>(applicationName, appVersion, k_engineName, k_engineVersion, k_vulkanVersion);

	//Create device
	vk::PhysicalDeviceFeatures desiredFeatures;
	vk::PhysicalDeviceProperties desiredProperties;
	desiredProperties.deviceType = vk::PhysicalDeviceType::eDiscreteGpu;
	//desiredProperties.deviceType = vk::PhysicalDeviceType::eIntegratedGpu;
	desiredProperties.apiVersion = k_vulkanVersion;

	m_pDevice = std::make_shared<GfxDevice>(m_pInstance->GetInstance(), desiredFeatures, desiredProperties, k_deviceExtensions, k_deviceLayers);

	//Load shaders
	vk::raii::ShaderModule fragmentShader = LoadShaderModule("triangle.frag.spv");
	vk::raii::ShaderModule vertexShader = LoadShaderModule("triangle.vert.spv");

	VkSurfaceKHR _surface;
	glfwCreateWindowSurface(*m_pInstance->GetInstance(), pWindow->Get(), nullptr, &_surface);
	m_surface = std::move(vk::raii::SurfaceKHR(m_pInstance->GetInstance(), _surface));
	m_swapChain = m_pDevice->CreateSwapChain(*m_surface, k_numFramesBuffered);

	//TODO detect depth surface formats
	vk::Format depthSurfaceFormat = vk::Format::eD16Unorm;
	auto [width, height] = pWindow->GetWindowSize();
	m_depthBuffer = m_pDevice->CreateDepthStencil(width, height, depthSurfaceFormat);

	//Create attachments
	//Attachments describe what image formats/target formats we want write to / read from
	//TODO detect render surface formats
	vk::Format renderSurfaceFormat = vk::Format::eB8G8R8A8Unorm;

	std::array<vk::AttachmentDescription, 2> renderPassAttachments;
	// Color output attachment
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
		vk::ImageLayout::ePresentSrcKHR, //initial
		vk::ImageLayout::ePresentSrcKHR //final
	);

	//Depth test attachment
	renderPassAttachments[1] = vk::AttachmentDescription(
		{},
		depthSurfaceFormat,
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eDepthStencilAttachmentOptimal
	);

	//std::vector<vk::SubpassDependency> renderPassDependencies = {
	//	//colorAttachmentDependency
	//	vk::SubpassDependency(
	//		VK_SUBPASS_EXTERNAL,
	//		0 /*dst subpass*/,
	//		vk::PipelineStageFlagBits::eBottomOfPipe/*srcStage*/,
	//		vk::PipelineStageFlagBits::eColorAttachmentOutput/*dstStage*/,
	//		vk::AccessFlagBits::eNone /*src access mask*/,
	//		vk::AccessFlagBits::eColorAttachmentWrite,
	//		{} /*dependency flags*/
	//	),
	//	//depthDependency
	//	vk::SubpassDependency(
	//		VK_SUBPASS_EXTERNAL,
	//		0,
	//		vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
	//		vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
	//		vk::AccessFlagBits::eNone,
	//		vk::AccessFlagBits::eDepthStencilAttachmentWrite,
	//		{}
	//	)
	//};

	std::array<vk::SubpassDependency, 2> renderPassDependencies = {
		//Transition swapchain image from final to initial
		vk::SubpassDependency(
			VK_SUBPASS_EXTERNAL,
			0,
			vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::AccessFlagBits::eMemoryRead,
			vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
			vk::DependencyFlagBits::eByRegion
		),
		//Transition swapchain image from initial to final
		vk::SubpassDependency(
			0,
			VK_SUBPASS_EXTERNAL,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
			vk::AccessFlagBits::eMemoryRead,
			vk::DependencyFlagBits::eByRegion
		)
	};

	//Add to renderpass
	m_renderPass = GfxPipelineBuilder::CreateRenderPass(
		m_pDevice->GetDevice(),
		renderPassAttachments,
		renderPassDependencies);

	//Now we have a renderpass defined we need to connect actual image resources to it
	m_frames = std::vector<GfxFrame>(m_swapChain.Size());

	for (uint32_t i = 0; i < m_swapChain.Size(); ++i)
	{
		std::array<vk::ImageView, 2> colorNDepth;
		colorNDepth[0] = m_swapChain.GetImageView(i);
		colorNDepth[1] = *m_depthBuffer.view;

		vk::FramebufferCreateInfo frameBufferCreateInfo(
			{},
			*m_renderPass,
			colorNDepth,
			width,
			height,
			1
		);

		m_frames[i].frameBuffer = vk::raii::Framebuffer(m_pDevice->GetDevice(), frameBufferCreateInfo);
		m_frames[i].aquireImageSemaphore = m_pDevice->CreateVkSemaphore();
		m_frames[i].readyToPresentSemaphore = m_pDevice->CreateVkSemaphore();
		m_frames[i].commandPool = m_pDevice->CreateGraphicsCommandPool();
		m_frames[i].commandBuffers = std::move(m_pDevice->CreateCommandBuffers(*m_frames[i].commandPool, 2/*num buffers*/));
		m_frames[i].renderCompleteFence = m_pDevice->CreateFence();
	}

	vk::PushConstantRange mvpMatrixPush(vk::ShaderStageFlagBits::eVertex, 0/*offset*/, sizeof(glm::mat4x4));
	m_pipeline.layout = GfxPipelineBuilder::CreatePipelineLayout(m_pDevice->GetDevice(), mvpMatrixPush, nullptr);

	GfxPipelineBuilder builder;
	builder._shaderStages.push_back(
		GfxPipelineBuilder::CreateShaderStageInfo(vk::ShaderStageFlagBits::eVertex, *vertexShader)
	);
	builder._shaderStages.push_back(
		GfxPipelineBuilder::CreateShaderStageInfo(vk::ShaderStageFlagBits::eFragment, *fragmentShader)
	);

	builder._inputAssembly = GfxPipelineBuilder::CreateInputAssemblyInfo(vk::PrimitiveTopology::eTriangleList);
	builder._viewport.setX(0.0f);
	builder._viewport.setY((float)height);
	builder._viewport.setWidth((float)width);
	builder._viewport.setHeight(-(float)height);
	builder._viewport.setMinDepth(0.0f);
	builder._viewport.setMaxDepth(1.0f);

	builder._scissor.setOffset({ 0, 0 }); 
	builder._scissor.setExtent({ width, height });

	builder._rasterizer = GfxPipelineBuilder::CreateRasterizationStateInfo(vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone);
	builder._depthStencil = GfxPipelineBuilder::CreateDepthStencilStateInfo(VK_TRUE, VK_TRUE, vk::CompareOp::eLess);
	builder._multisampling = GfxPipelineBuilder::CreateMultisampleStateInfo();
	builder._colorBlendAttachment = GfxPipelineBuilder::CreateColorBlendAttachmentState();
	builder._pipelineLayout = *m_pipeline.layout;
	builder._vertexDescription = Vertex::GetDescription();

	m_pipeline.pipeline = builder.BuildPipeline(m_pDevice->GetDevice(), *m_renderPass);

	//Todo move scene loading somewhere else
	// good rust candidate?
	Mesh pirate = ModelLoader::LoadModel("C:/Users/Jarryd/Projects/vulkan-gpugems/assets/pirate.obj");
	m_modelVertexBuffer = m_pDevice->CreateBuffer(pirate.vertices.size() * sizeof(Vertex), vk::BufferUsageFlagBits::eVertexBuffer);
	m_modelIndexBuffer = m_pDevice->CreateBuffer(pirate.indices.size() * sizeof(uint32_t), vk::BufferUsageFlagBits::eIndexBuffer);

	memcpy(m_modelVertexBuffer.m_pData, pirate.vertices.data(), m_modelVertexBuffer.m_dataSize);
	memcpy(m_modelIndexBuffer.m_pData, pirate.indices.data(), m_modelIndexBuffer.m_dataSize);

	//TODO move out
	m_timingQueryPool = m_pDevice->CreateQueryPool(k_queryPoolCount);


	//TODO move out
	vk::raii::ShaderModule textVertShader = LoadShaderModule("text.vert.spv");
	vk::raii::ShaderModule textFragShader = LoadShaderModule("text.frag.spv");
	m_textOverlay = GfxTextOverlay(m_pDevice, *m_frames[0].commandPool, builder._viewport, builder._scissor, *textVertShader, *textFragShader);
}

GfxEngine::~GfxEngine()
{
	m_pDevice->GetDevice().waitIdle();
}

void GfxEngine::Render()
{
	double frameCpuBeginTime = glfwGetTime() * 1000;

	uint64_t const k_aquireTimeout_ns = 100000000; //0.1 seconds
	uint64_t const k_renderCompleteTimeout_ns = 1000000000; //1 second
	GfxFrame& frame = GetCurrentFrame();
	m_pDevice->GetDevice().waitForFences(*frame.renderCompleteFence, VK_TRUE /*wait all*/, k_renderCompleteTimeout_ns);
	m_pDevice->GetDevice().resetFences(*frame.renderCompleteFence);

	//TODO verify timing results somehow?
	//Grab previous frame perf results
	auto [queryResult, resultData] = m_timingQueryPool.getResults<double>(
		0/*first query*/,
		2 /*queryCount*/,
		sizeof(uint64_t) * 2/*assuming 64 bit per result we want*/,
		sizeof(uint64_t),
		vk::QueryResultFlagBits::e64);

	double frameGpuBeginTime = resultData[0] * m_pDevice->GetProperties().limits.timestampPeriod *1e+6; //timestamp period changes to ns e+6 takes us to ms
	double frameGpuEndTime = resultData[1] * m_pDevice->GetProperties().limits.timestampPeriod *1e+6;

	auto [result, imageIndex] = m_swapChain.m_swapchain.acquireNextImage(k_aquireTimeout_ns, *frame.aquireImageSemaphore);//TODO: check and handle failed aquisition

	frame.commandPool.reset();
	vk::CommandBufferBeginInfo const beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	frame.commandBuffers[0].begin(beginInfo);

	frame.commandBuffers[0].resetQueryPool(*m_timingQueryPool, 0, k_queryPoolCount);
	frame.commandBuffers[0].writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, *m_timingQueryPool, 0);

	//TODO wait on image transition manually as subpass dependencies don't seem to be waiting?

	vk::ClearColorValue const k_clearColor(std::array<float, 4>{48.0f / 2550.f, 10.0f / 255.0f, 36.0f / 255.0f, 1.0f});
	vk::ClearDepthStencilValue const k_depthClear(1.0f, 0); //1.0 is max depth
	std::array<vk::ClearValue, 2> clearValues = { k_clearColor, k_depthClear };

	vk::Rect2D const renderArea({ 0,0 }, m_swapChain.m_extent);
	vk::RenderPassBeginInfo passBeginInfo(*m_renderPass, *frame.frameBuffer, renderArea, clearValues);

	//Rotate model
	glm::mat4 const model = glm::rotate(glm::identity<glm::mat4>(), glm::radians(m_numFramesRendered * 0.4f), glm::vec3(0.5f,0.5f, 0.0f));
	glm::mat4 const mvp = m_camera.GetViewProj() * model;

	frame.commandBuffers[0].beginRenderPass(passBeginInfo, vk::SubpassContents::eInline);
	frame.commandBuffers[0].bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline.pipeline);
	frame.commandBuffers[0].bindVertexBuffers(0/*first binding*/, *m_modelVertexBuffer.m_buffer, { 0 } /*offset*/);
	frame.commandBuffers[0].bindIndexBuffer(*m_modelIndexBuffer.m_buffer, 0/*offset*/, vk::IndexType::eUint32);

	glm::mat4 model2 = glm::translate(glm::identity<glm::mat4>(), glm::vec3(-0.5, -0.5f, -0.5f));
	model2 = glm::scale(model2, glm::vec3(0.1f, 0.1f, 0.1f));
	glm::mat4 const mvp2 = m_camera.GetViewProj() * model2;
	frame.commandBuffers[0].pushConstants<glm::mat4>(*m_pipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, mvp2);
	frame.commandBuffers[0].drawIndexed(m_modelIndexBuffer.m_dataSize / sizeof(uint32_t), 1/*instance count*/, 0, 0, 0);

	for (int i = 0; i < 100; ++i)
	{
		frame.commandBuffers[0].pushConstants<glm::mat4>(*m_pipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, mvp);
		frame.commandBuffers[0].drawIndexed(m_modelIndexBuffer.m_dataSize / sizeof(uint32_t), 1/*instance count*/, 0, 0, 0);
	}
	
	frame.commandBuffers[0].endRenderPass();

	frame.commandBuffers[0].writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, *m_timingQueryPool, 1);
	frame.commandBuffers[0].end();

	m_textOverlay.RenderTextOverlay(frame, renderArea, *frame.commandBuffers[1]);

	vk::PipelineStageFlags const submitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::Queue const queue = m_pDevice->GetGraphicsQueue();
	std::vector<vk::CommandBuffer> submitted{ *frame.commandBuffers[0], *frame.commandBuffers[1] };
	vk::SubmitInfo renderSubmitInfo(*frame.aquireImageSemaphore, submitStageMask, submitted, *frame.readyToPresentSemaphore);
	queue.submit(renderSubmitInfo, *frame.renderCompleteFence);

	vk::PresentInfoKHR presentInfo(*frame.readyToPresentSemaphore, *m_swapChain.m_swapchain, imageIndex);
	queue.presentKHR(presentInfo);//TODO handle different Success results

	//Perf updates
	double frameCpuEndTime = glfwGetTime() * 1000;

	m_pWindow->SetTitle(std::format("cpu: {0:.3f}ms gpu: {1:.3f}ms", frameCpuEndTime - frameCpuBeginTime, frameGpuEndTime - frameGpuBeginTime));

	m_numFramesRendered++;
}

vk::raii::ShaderModule GfxEngine::LoadShaderModule(std::string const& filePath)
{
	//TODO RAII files
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

GfxFrame& GfxEngine::GetCurrentFrame()
{
	return m_frames[m_numFramesRendered % k_numFramesBuffered];
}
