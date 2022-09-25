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

//TODO move use of static model to a bucket system
#include "StaticModel.h"

//TODO wrap extensions and layers into configurable features?
std::vector<const char*> const k_deviceExtensions{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
};

std::vector<const char*> const k_deviceLayers{
	//Deprecated, but might be needed for backwards compatibility if we ever want it
};

std::vector<glm::vec3> const k_lightDirection{
	glm::vec3(0.0f, -2.0f, 0.0f)
};

glm::vec3 const k_light(0.0f, -2.0f, 0.0f);

constexpr uint32_t k_lightBindingId = 0;
constexpr uint32_t k_transformBindingId = 0;
constexpr uint32_t k_maxModelTransforms = 10000;

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
	, m_pMeshPool(std::make_shared<MeshPool>())
	, m_models()
	, m_camera(pWindow->GetWindowWidth(), pWindow->GetWindowHeight())
	, m_lightBuffer()
	, m_transformBuffer()
	, m_numFramesRendered(0)
	, m_pDescriptorManager(nullptr)
	, m_timingQueryPool(nullptr)
	, m_samplers()
{

	m_pInstance = std::make_shared<GfxApiInstance>(applicationName, appVersion, k_engineName, k_engineVersion, k_vulkanVersion);

	//Create device
	vk::PhysicalDeviceFeatures2 desiredFeatures;
	vk::PhysicalDeviceShaderDrawParametersFeatures shaderDrawParamsFeatures(VK_TRUE, nullptr);
	desiredFeatures.setPNext(&shaderDrawParamsFeatures);

	vk::PhysicalDeviceProperties desiredProperties;
	desiredProperties.deviceType = vk::PhysicalDeviceType::eDiscreteGpu;
	//desiredProperties.deviceType = vk::PhysicalDeviceType::eIntegratedGpu;
	desiredProperties.apiVersion = k_vulkanVersion;

	m_pDevice = std::make_shared<GfxDevice>(m_pInstance->GetInstance(), desiredFeatures, desiredProperties, k_deviceExtensions, k_deviceLayers);
	m_pDescriptorManager = std::make_unique<GfxDescriptorManager>(m_pDevice);

	//Load shaders
	vk::raii::ShaderModule fragmentShader = LoadShaderModule("gooch.frag.spv");
	vk::raii::ShaderModule vertexShader = LoadShaderModule("gooch.vert.spv");

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

	//Model variables
	SPDLOG_INFO("Loading Scene");

	//Todo move scene loading somewhere else
	// good rust candidate?
	m_models.emplace_back(std::make_shared<StaticModel>(m_pDevice, m_pMeshPool, "C:/Users/Jarryd/Projects/vulkan-gpugems/assets/pirate.obj"));
	m_models.emplace_back(std::make_shared<StaticModel>(m_pDevice, m_pMeshPool, "C:/Users/Jarryd/Projects/vulkan-gpugems/assets/pirate.obj"));


	SPDLOG_INFO("Constructing descriptor sets");

	m_pDescriptorManager->SetBinding(k_transformBindingId, vk::ShaderStageFlagBits::eVertex, DataUsageFrequency::ePerModel, vk::DescriptorType::eStorageBuffer);
	m_transformBuffer = m_pDevice->CreateBuffer(sizeof(m_models.at(0)->GetTransform()) * k_maxModelTransforms, vk::BufferUsageFlagBits::eStorageBuffer);

	//Lights
	m_pDescriptorManager->SetBinding(k_lightBindingId, vk::ShaderStageFlagBits::eFragment, DataUsageFrequency::ePerFrame, vk::DescriptorType::eUniformBuffer);

	m_lightBuffer = m_pDevice->CreateBuffer(sizeof(k_light), vk::BufferUsageFlagBits::eUniformBuffer);
	vk::DescriptorBufferInfo lightBufferInfo(*m_lightBuffer.m_buffer, 0, m_lightBuffer.m_dataSize);
	vk::WriteDescriptorSet writeDescriptor(
		m_pDescriptorManager->GetDescriptor(DataUsageFrequency::ePerFrame),
		k_lightBindingId,
		0 /*array element*/,
		vk::DescriptorType::eUniformBuffer,
		nullptr, lightBufferInfo, nullptr);
	m_pDevice->GetDevice().updateDescriptorSets(writeDescriptor, nullptr);
	memcpy(m_lightBuffer.m_pData, &k_light, m_lightBuffer.m_dataSize);

	SPDLOG_INFO("Constructing Pipeline");
	
	vk::DescriptorSetLayout lightLayout = m_pDescriptorManager->GetLayout(DataUsageFrequency::ePerFrame);
	vk::DescriptorSetLayout transformLayout = m_pDescriptorManager->GetLayout(DataUsageFrequency::ePerModel);
	std::vector<vk::DescriptorSetLayout> layouts = { lightLayout, transformLayout};
	m_pipeline.layout = GfxPipelineBuilder::CreatePipelineLayout(m_pDevice->GetDevice(), nullptr, layouts);

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

	double frameGpuBeginTime = resultData[0] * m_pDevice->GetProperties().limits.timestampPeriod * 1e+6; //timestamp period changes to ns e+6 takes us to ms
	double frameGpuEndTime = resultData[1] * m_pDevice->GetProperties().limits.timestampPeriod * 1e+6;

	//Upload Model transforms at the start of every frame
	for (uint32_t i = 0; i < m_models.size(); ++i)
	{
		uint32_t singleBufferSize = sizeof(ObjectData);
		StaticModelPtr_t const& pModel = m_models.at(i);
		//TODO move camera info and this multiplication to shader and UBO uploads?
		glm::mat4 const mvp = m_camera.GetViewProj() * pModel->GetTransform();
		uint8_t* destination = static_cast<uint8_t*>(m_transformBuffer.m_pData) + (singleBufferSize * i);
		memcpy(destination, &mvp, sizeof(mvp));
	}

	auto [result, imageIndex] = m_swapChain.m_swapchain.acquireNextImage(k_aquireTimeout_ns, *frame.aquireImageSemaphore);//TODO: check and handle failed aquisition

	//TODO remove after finished prototyping bindless
	m_pDevice->GetDevice().waitIdle();

	frame.commandPool.reset();
	vk::CommandBufferBeginInfo const beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	frame.commandBuffers[0].begin(beginInfo);

	frame.commandBuffers[0].resetQueryPool(*m_timingQueryPool, 0, k_queryPoolCount);
	frame.commandBuffers[0].writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, *m_timingQueryPool, 0);

	vk::ClearColorValue const k_clearColor(std::array<float, 4>{48.0f / 2550.f, 10.0f / 255.0f, 36.0f / 255.0f, 1.0f});
	vk::ClearDepthStencilValue const k_depthClear(1.0f, 0); //1.0 is max depth
	std::array<vk::ClearValue, 2> clearValues = { k_clearColor, k_depthClear };

	vk::Rect2D const renderArea({ 0,0 }, m_swapChain.m_extent);
	vk::RenderPassBeginInfo passBeginInfo(*m_renderPass, *frame.frameBuffer, renderArea, clearValues);

	//TODO move model translation to scene logic processing
	m_models.at(0)->SetRotation(m_numFramesRendered * 0.4f, glm::vec3(0.5f, 0.5f, 0.0f));
	m_models.at(1)->SetPosition(glm::vec3(-0.5, -0.5f, -0.5f));

	//Copy data to descriptor sets before binding
	for (uint32_t i = 0; i < m_models.size(); ++i)
	{
		uint32_t singleBufferStride = m_transformBuffer.m_dataSize / m_models.size();
		vk::DescriptorBufferInfo transformBufferInfo(*m_transformBuffer.m_buffer, 0, singleBufferStride);
		vk::WriteDescriptorSet writeDescriptor(
			m_pDescriptorManager->GetDescriptor(DataUsageFrequency::ePerModel),
			k_transformBindingId,
			0 /*array element*/,
			vk::DescriptorType::eStorageBuffer, //TODO descriptor manager should give us the "location" details for a descriptor binding to write
			nullptr, transformBufferInfo, nullptr);
		m_pDevice->GetDevice().updateDescriptorSets(writeDescriptor, nullptr);
	}

	frame.commandBuffers[0].bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		*m_pipeline.layout,
		0,
		{m_pDescriptorManager->GetDescriptor(DataUsageFrequency::ePerFrame), m_pDescriptorManager->GetDescriptor(DataUsageFrequency::ePerModel)}, //Bind per model here as we are going bindless
		nullptr
	);

	frame.commandBuffers[0].beginRenderPass(passBeginInfo, vk::SubpassContents::eInline);
	frame.commandBuffers[0].bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline.pipeline);

	for (uint32_t i = 0; i < m_models.size(); ++i)
	{
		StaticModelPtr_t const& pModel = m_models[i];
		frame.commandBuffers[0].bindVertexBuffers(0/*first binding*/, *pModel->GetVertexBuffer().m_buffer, { 0 } /*offset*/);
		frame.commandBuffers[0].bindIndexBuffer(*pModel->GetIndexBuffer().m_buffer, 0/*offset*/, vk::IndexType::eUint32);
		frame.commandBuffers[0].drawIndexed(pModel->GetIndexBuffer().m_dataSize / sizeof(uint32_t), 1/*instance count*/, 0, 0, i /*first instance*/);
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
