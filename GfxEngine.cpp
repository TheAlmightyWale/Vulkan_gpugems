#include "GfxEngine.h"
#include "GfxFrame.h"
#include "GfxDevice.h"
#include "GfxImage.h"
#include "GfxPipeline.h"
#include "GfxPipelineBuilder.h"
#include "GfxSwapchain.h"
#include "Logger.h"
#include "Exceptions.h"

#include "StaticModel.h"
#include "ImageLoader.h"

#include "ShaderLoader.h"

//TODO move out once rendering and terrain generation are separated
#include "TerrainGenerator.h"

//TODO wrap extensions and layers into configurable features?
std::vector<const char*> const k_deviceExtensions{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
};

std::vector<const char*> const k_deviceLayers{
	//Deprecated, but might be needed for backwards compatibility if we ever want it
};

glm::vec3 const k_light(-1.0f, 0.0f, -8.0f);

struct FrameData {
	glm::vec4 directionalLight;
	glm::vec4 cameraPosition;
};

constexpr uint32_t k_lightBindingId = 0;
constexpr uint32_t k_objectDataBindingId = 0;
constexpr uint32_t k_textureBindingId = 0;

constexpr uint32_t k_modelCount = 8;
constexpr uint32_t k_cubeCount = 12;

GfxEngine::GfxEngine(std::string const& applicationName, uint32_t appVersion, WindowPtr_t pWindow, std::shared_ptr<ObjectProcessor> pObjectProcessor)
	: m_pInstance(nullptr)
	, m_pWindow(pWindow)
	, m_pDevice(nullptr)
	, m_surface(nullptr)
	, m_swapChain()
	, m_renderPass(nullptr)
	, m_frames()
	, m_depthBuffer()
	, m_pipeline()
	, m_goochPipeline()
	, m_textOverlay()
	, m_pMeshPool(std::make_shared<MeshPool>())
	, m_models()
	, m_textureImage()
	, m_textureSampler(nullptr)
	, m_pCamera(std::make_shared<Camera>(pWindow->GetWindowWidth(), pWindow->GetWindowHeight()))
	, m_frameDataBuffer()
	, m_objectDataBuffer()
	, m_numFramesRendered(0)
	, m_pDescriptorManager(nullptr)
	, m_pGoochDescriptorManager(nullptr)
	, m_timingQueryPool(nullptr)
	, m_pObjectProcessor(pObjectProcessor)
	, m_pTerrain(nullptr)
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
	// TODO manage pipelines for different shader needs
	vk::raii::ShaderModule goochFragmentShader = ShaderLoader::LoadModule("gooch.frag.spv", m_pDevice);
	vk::raii::ShaderModule goochVertexShader = ShaderLoader::LoadModule("gooch.vert.spv", m_pDevice);
	vk::raii::ShaderModule phongFragmentShader = ShaderLoader::LoadModule("blinnPhong.frag.spv", m_pDevice);
	vk::raii::ShaderModule phongVertexShader = ShaderLoader::LoadModule("blinnPhong.vert.spv", m_pDevice);

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

	//TODO remove eventually when we have a scene loader
	m_pObjectProcessor->SetCamera(m_pCamera);

	//Todo move scene loading somewhere else
	// good rust candidate?
	for (uint32_t i = 0; i < k_modelCount; ++i)
	{
		auto pModel = std::make_shared<StaticModel>(m_pDevice, m_pMeshPool, "C:/Users/Jarryd/Projects/vulkan-gpugems/assets/pirate.obj", m_pObjectProcessor);
		m_models.emplace_back(pModel);
	
		//place meshes in a gently rising grid
		float const k_modelSpacing = 2.0f;
		glm::vec3 position(i % 3 * k_modelSpacing, i * k_modelSpacing, i % 3 * k_modelSpacing);
		pModel->SetPosition(position);
	}

	for (uint32_t i = 0; i < k_cubeCount; ++i)
	{
		auto pCube = std::make_shared<StaticModel>(m_pDevice, m_pMeshPool, "C:/Users/Jarryd/Projects/vulkan-gpugems/assets/cube.obj", m_pObjectProcessor);
		m_models.emplace_back(pCube);

		//place cubes in an offset grid
		float const k_cubeSpacing = 2.0f;
		glm::vec3 position(i % 3 * k_cubeSpacing, i * k_cubeSpacing * 0.5f, i % 4 * k_cubeSpacing + 5.0f);
		pCube->SetPosition(position);
	}

	SPDLOG_INFO("Constructing descriptor sets");

	//Per object data
	m_pDescriptorManager->AddBinding(k_objectDataBindingId, vk::ShaderStageFlagBits::eVertex, DataUsageFrequency::ePerModel, vk::DescriptorType::eStorageBuffer);
	constexpr size_t k_objectDataBufferSize = sizeof(ObjectData) * k_maxModelTransforms + sizeof(CameraShaderData);//Only taking one camera into account
	m_objectDataBuffer = m_pDevice->CreateBuffer(k_objectDataBufferSize, vk::BufferUsageFlagBits::eStorageBuffer);

	//Per frame data
	m_pDescriptorManager->AddBinding(k_lightBindingId, vk::ShaderStageFlagBits::eFragment, DataUsageFrequency::ePerFrame, vk::DescriptorType::eUniformBuffer);
	m_frameDataBuffer = m_pDevice->CreateBuffer(sizeof(FrameData), vk::BufferUsageFlagBits::eUniformBuffer);

	//Per material data
	m_pDescriptorManager->AddBinding(k_textureBindingId, vk::ShaderStageFlagBits::eFragment, DataUsageFrequency::ePerMaterial, vk::DescriptorType::eCombinedImageSampler);
	//Load Texture image
	ImagePtr_t pImage = ImageLoader::LoadTexture(*m_pDevice, "C:/Users/Jarryd/Projects/vulkan-gpugems/assets/fish.png");

	vk::ImageCreateInfo textureCreateInfo(
		{},
		vk::ImageType::e2D,
		pImage->format,
		vk::Extent3D{
			pImage->width,
			pImage->height,
			1
		},
		1 /*Mip levels*/,
		1 /*array levels*/,
		vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		vk::SharingMode::eExclusive
	);
	m_textureImage = m_pDevice->CreateImage(textureCreateInfo, vk::ImageAspectFlagBits::eColor, vk::MemoryPropertyFlagBits::eDeviceLocal);
	m_pDevice->UploadImageData(*m_frames[0].commandPool, m_pDevice->GetGraphicsQueue(), m_textureImage, pImage->data);

	//Create Texture Sampler and bind it
	vk::SamplerCreateInfo samplerCreateInfo(
		{},
		vk::Filter::eLinear,
		vk::Filter::eLinear,
		vk::SamplerMipmapMode::eLinear,
		/*address modes for U,V,W respectively*/
		vk::SamplerAddressMode::eMirroredRepeat,
		vk::SamplerAddressMode::eMirroredRepeat,
		vk::SamplerAddressMode::eMirroredRepeat,
		0.0f /*mip LOD bias*/,
		VK_FALSE /*Anisotropy enable*/,
		0.0f /*max anisotrophy*/,
		VK_FALSE /*compare enable*/,
		vk::CompareOp::eNever,
		0.0f /*min LOD*/,
		1.0f /*max LOD*/,
		vk::BorderColor::eFloatOpaqueWhite
	);
	m_textureSampler = vk::raii::Sampler(m_pDevice->GetDevice(), samplerCreateInfo);
	vk::DescriptorImageInfo textureDescriptor(*m_textureSampler, *m_textureImage.view, vk::ImageLayout::eShaderReadOnlyOptimal);
	vk::WriteDescriptorSet samplerWrite = m_pDescriptorManager->GetWriteDescriptor(DataUsageFrequency::ePerMaterial, k_textureBindingId);
	samplerWrite.setPImageInfo(&textureDescriptor);
	samplerWrite.setDescriptorCount(1);
	m_pDevice->GetDevice().updateDescriptorSets(samplerWrite, nullptr);

	vk::Viewport viewport(0.0f, (float)height, (float)width, -(float)height, 0.0f, 1.0f);

	SPDLOG_INFO("Constructing Gooch Pipeline");
	m_pGoochDescriptorManager = std::make_unique<GfxDescriptorManager>(m_pDevice);

	m_pGoochDescriptorManager->AddBinding(k_objectDataBindingId, vk::ShaderStageFlagBits::eVertex, DataUsageFrequency::ePerModel, vk::DescriptorType::eStorageBuffer);
	m_pGoochDescriptorManager->AddBinding(k_lightBindingId, vk::ShaderStageFlagBits::eFragment, DataUsageFrequency::ePerFrame, vk::DescriptorType::eUniformBuffer);

	vk::DescriptorSetLayout goochFrameLayout = m_pGoochDescriptorManager->GetLayout(DataUsageFrequency::ePerFrame);
	vk::DescriptorSetLayout goochModelLayout = m_pGoochDescriptorManager->GetLayout(DataUsageFrequency::ePerModel);
	std::vector<vk::DescriptorSetLayout> goochlayouts = { goochFrameLayout, goochModelLayout };

	m_goochPipeline.layout = GfxPipelineBuilder::CreatePipelineLayout(m_pDevice->GetDevice(), nullptr, goochlayouts);

	GfxPipelineBuilder goochBuilder;
	goochBuilder._shaderStages.push_back(GfxPipelineBuilder::CreateShaderStageInfo(vk::ShaderStageFlagBits::eVertex, *goochVertexShader));
	goochBuilder._shaderStages.push_back(GfxPipelineBuilder::CreateShaderStageInfo(vk::ShaderStageFlagBits::eFragment, *goochFragmentShader));
	goochBuilder._inputAssembly = GfxPipelineBuilder::CreateInputAssemblyInfo(vk::PrimitiveTopology::eTriangleList);
	goochBuilder._viewport = viewport;
	goochBuilder._scissor.setOffset({ 0,0 });
	goochBuilder._scissor.setExtent({ width, height });
	goochBuilder._rasterizer = GfxPipelineBuilder::CreateRasterizationStateInfo(vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone);
	goochBuilder._depthStencil = GfxPipelineBuilder::CreateDepthStencilStateInfo(VK_TRUE, VK_TRUE, vk::CompareOp::eLess);
	goochBuilder._multisampling = GfxPipelineBuilder::CreateMultisampleStateInfo();
	goochBuilder._colorBlendAttachment = GfxPipelineBuilder::CreateColorBlendAttachmentState();
	goochBuilder._pipelineLayout = *m_goochPipeline.layout;
	goochBuilder._vertexDescription = Vertex::GetDescription();

	m_goochPipeline.pipeline = goochBuilder.BuildPipeline(m_pDevice->GetDevice(), *m_renderPass);

	SPDLOG_INFO("Constructing Phong Pipeline");

	vk::DescriptorSetLayout lightLayout = m_pDescriptorManager->GetLayout(DataUsageFrequency::ePerFrame);
	vk::DescriptorSetLayout transformLayout = m_pDescriptorManager->GetLayout(DataUsageFrequency::ePerModel);
	vk::DescriptorSetLayout textureLayout = m_pDescriptorManager->GetLayout(DataUsageFrequency::ePerMaterial);
	std::vector<vk::DescriptorSetLayout> layouts = { lightLayout, transformLayout, textureLayout };
	m_pipeline.layout = GfxPipelineBuilder::CreatePipelineLayout(m_pDevice->GetDevice(), nullptr, layouts);

	GfxPipelineBuilder builder;
	builder._shaderStages.push_back(
		GfxPipelineBuilder::CreateShaderStageInfo(vk::ShaderStageFlagBits::eVertex, *phongVertexShader)
	);
	builder._shaderStages.push_back(
		GfxPipelineBuilder::CreateShaderStageInfo(vk::ShaderStageFlagBits::eFragment, *phongFragmentShader)
	);

	builder._inputAssembly = GfxPipelineBuilder::CreateInputAssemblyInfo(vk::PrimitiveTopology::eTriangleList);
	builder._viewport = viewport;

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

	m_textOverlay = GfxTextOverlay(m_pDevice, *m_frames[0].commandPool, builder._viewport, builder._scissor);

	m_pTerrain = std::make_shared<TerrainGenerator>(m_pDevice, viewport, builder._scissor, *m_renderPass);
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

	auto [acquireResult, imageIndex] = m_swapChain.m_swapchain.acquireNextImage(k_aquireTimeout_ns, *frame.aquireImageSemaphore);//TODO: check and handle failed aquisition

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

	//Copy data to gpu before binding descriptor set
	UploadFrameDataToGpu();
	UploadObjectDataToGpu();

	//frame.commandBuffers[0].bindDescriptorSets(
	//	vk::PipelineBindPoint::eGraphics,
	//	*m_pipeline.layout,
	//	0,
	//	{ m_pDescriptorManager->GetDescriptor(DataUsageFrequency::ePerFrame),
	//	m_pDescriptorManager->GetDescriptor(DataUsageFrequency::ePerModel),
	//	m_pDescriptorManager->GetDescriptor(DataUsageFrequency::ePerMaterial)}, //Bind per model and material here as we are going bindless
	//	nullptr
	//);
	//
	//frame.commandBuffers[0].beginRenderPass(passBeginInfo, vk::SubpassContents::eInline);
	//
	//frame.commandBuffers[0].bindPipeline(vk::PipelineBindPoint::eGraphics, *m_goochPipeline.pipeline);
	//for (uint32_t i = 0; i < k_modelCount; ++i)
	//{
	//	StaticModelPtr_t const& pModel = m_models[i];
	//	frame.commandBuffers[0].bindVertexBuffers(0/*first binding*/, *pModel->GetVertexBuffer().m_buffer, { 0 } /*offset*/);
	//	frame.commandBuffers[0].bindIndexBuffer(*pModel->GetIndexBuffer().m_buffer, 0/*offset*/, vk::IndexType::eUint32);
	//	frame.commandBuffers[0].drawIndexed(pModel->GetIndexBuffer().m_dataSize / sizeof(uint32_t), 1/*instance count*/, 0, 0, i /*first instance*/);
	//}
	//
	//frame.commandBuffers[0].bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline.pipeline);
	//
	//for (uint32_t i = 0; i < k_cubeCount; ++i)
	//{
	//	uint32_t modelIndex = i + k_modelCount;
	//	StaticModelPtr_t const& pModel = m_models[modelIndex];
	//	frame.commandBuffers[0].bindVertexBuffers(0/*first binding*/, *pModel->GetVertexBuffer().m_buffer, { 0 } /*offset*/);
	//	frame.commandBuffers[0].bindIndexBuffer(*pModel->GetIndexBuffer().m_buffer, 0/*offset*/, vk::IndexType::eUint32);
	//	frame.commandBuffers[0].drawIndexed(pModel->GetIndexBuffer().m_dataSize / sizeof(uint32_t), 1/*instance count*/, 0, 0, modelIndex /*first instance*/);
	//}
	//
	//frame.commandBuffers[0].endRenderPass();
	//
	//frame.commandBuffers[0].writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, *m_timingQueryPool, 1);
	frame.commandBuffers[0].end();

	m_textOverlay.RenderTextOverlay(frame, renderArea, *frame.commandBuffers[1]);

	vk::CommandBuffer terrainCommandBuffer = m_pTerrain->Render(*m_renderPass, passBeginInfo, m_pDevice);

	vk::PipelineStageFlags const submitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::Queue const queue = m_pDevice->GetGraphicsQueue();
	std::vector<vk::CommandBuffer> submitted{ *frame.commandBuffers[0], terrainCommandBuffer, *frame.commandBuffers[1] };

	if (m_pTerrain->ReadyToRender())
	{
		submitted.push_back(m_pTerrain->RenderTerrain(*m_renderPass, passBeginInfo, m_pDevice, *m_pCamera));
	}

	vk::SubmitInfo renderSubmitInfo(*frame.aquireImageSemaphore, submitStageMask, submitted, *frame.readyToPresentSemaphore);
	queue.submit(renderSubmitInfo, *frame.renderCompleteFence);

	vk::PresentInfoKHR presentInfo(*frame.readyToPresentSemaphore, *m_swapChain.m_swapchain, imageIndex);
	queue.presentKHR(presentInfo);//TODO handle different Success results

	//Temp for density checking
	queue.waitIdle();
	std::vector<float> densityResult = m_pTerrain->GetDensityOutput();
	m_pTerrain->GenerateVertexBuffer(densityResult);

	//Perf updates
	double frameCpuEndTime = glfwGetTime() * 1000;

	m_pWindow->SetTitle(std::format("cpu: {0:.3f}ms gpu: {1:.3f}ms", frameCpuEndTime - frameCpuBeginTime, frameGpuEndTime - frameGpuBeginTime));

	m_numFramesRendered++;
}

GfxFrame& GfxEngine::GetCurrentFrame()
{
	return m_frames[m_numFramesRendered % k_numFramesBuffered];
}

size_t GfxEngine::PrepObjectDataForUpload()
{
	//First upload Camera data
	size_t writeOffset = m_objectDataBuffer.CopyToBuffer(&m_pCamera->GetViewProj(), sizeof(CameraShaderData), 0);

	//Upload Model transforms at the start of every frame
	for (uint32_t i = 0; i < m_models.size(); ++i)
	{
		//TODO exoise ObjectData contiguous store somehow, so we don't need to iterate through poiunter redirectes
		writeOffset = m_objectDataBuffer.CopyToBuffer(&m_models.at(i)->GetTransform(), sizeof(ObjectData), writeOffset);
	}

	//TODO Currently have no way of knowing where the starting offset of copyToBuffer is
	// which will be needed once this buffer copy gets shared between more than just this function
	return writeOffset;
}

void GfxEngine::UploadObjectDataToGpu()
{
	size_t totalObjectDataBytesToUpload = PrepObjectDataForUpload();
	vk::WriteDescriptorSet writeDescriptor = m_pDescriptorManager->GetWriteDescriptor(DataUsageFrequency::ePerModel, k_objectDataBindingId);
	m_pDevice->UploadBufferData(totalObjectDataBytesToUpload, 0, *m_objectDataBuffer.m_buffer, writeDescriptor);
}

void GfxEngine::UploadFrameDataToGpu()
{
	FrameData data;
	data.directionalLight = glm::vec4(k_light, 1.0f);
	data.cameraPosition = glm::vec4(m_pCamera->GetPosition(), 1.0f);
	m_frameDataBuffer.CopyToBuffer(&data, sizeof(FrameData), 0);

	vk::WriteDescriptorSet writeDescriptor = m_pDescriptorManager->GetWriteDescriptor(DataUsageFrequency::ePerFrame, k_objectDataBindingId);
	m_pDevice->UploadBufferData(sizeof(FrameData), 0, *m_frameDataBuffer.m_buffer, writeDescriptor);
}