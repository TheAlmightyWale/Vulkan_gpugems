#include "GfxTextOverlay.h"
#include "GfxDevice.h"
#include "GfxFrame.h"
#include "GfxPipelineBuilder.h"
#include "Mesh.h"
#include "Math.h"
#include "Exceptions.h"
#include "Logger.h"

GfxTextOverlay::GfxTextOverlay()
	: overlayPipeline(nullptr)
	, overlayRenderPass(nullptr)
	, textOverlayCommandBuffers(nullptr)
	, textImage()
	, sampler(nullptr)
	, descriptorPool(nullptr)
	, overlayDescriptorLayout(nullptr)
	, overlaySet(nullptr)
	, overlayLayout(nullptr)
	, overlayVertexBuffer()
	, overlayFrameBuffers({ nullptr, nullptr })
{}

GfxTextOverlay::GfxTextOverlay(
	GfxDevicePtr_t pDevice,
	vk::CommandPool graphicsCommandPool,
	vk::Viewport viewport,
	vk::Rect2D scissor,
	vk::ShaderModule textVertShader,
	vk::ShaderModule textFragShader)
	: overlayPipeline(nullptr)
	, overlayRenderPass(nullptr)
	, textOverlayCommandBuffers(nullptr)
	, textImage()
	, sampler(nullptr)
	, descriptorPool(nullptr)
	, overlayDescriptorLayout(nullptr)
	, overlaySet(nullptr)
	, overlayLayout(nullptr)
	, overlayVertexBuffer()
	, overlayFrameBuffers({nullptr, nullptr})
{
	SPDLOG_INFO("Creating Text Overlay");

	//Load font
	uint32_t const k_fontWidth = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;
	uint32_t const k_fontHeight = STB_FONT_consolas_24_latin1_BITMAP_WIDTH;

	static uint8_t font24Pixels[k_fontWidth][k_fontHeight];
	stb_font_consolas_24_latin1(stbFontData, font24Pixels, k_fontHeight);

	textOverlayCommandBuffers = std::move(pDevice->CreateCommandBuffers(graphicsCommandPool, 2));

	overlayVertexBuffer = pDevice->CreateBuffer(k_max_char_count * sizeof(glm::vec4), vk::BufferUsageFlagBits::eVertexBuffer);

	vk::Format textImageFormat = vk::Format::eR8Unorm;
	vk::ImageCreateInfo createInfo(
		{} /*flags*/,
		vk::ImageType::e2D,
		textImageFormat,
		vk::Extent3D{
			k_fontWidth,
			k_fontHeight,
			1
		},
		1 /*Mip levels*/,
		1 /*Array levels*/,
		vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		vk::SharingMode::eExclusive
	);

	textImage = pDevice->CreateImage(createInfo, vk::ImageAspectFlagBits::eColor, vk::MemoryPropertyFlagBits::eDeviceLocal);

	//Stage data
	vk::DeviceSize size = textImage.image.getMemoryRequirements().size;
	GfxBuffer stagingBuffer = pDevice->CreateBuffer(size, vk::BufferUsageFlagBits::eTransferSrc);
	memcpy(stagingBuffer.m_pData, &font24Pixels[0][0], static_cast<size_t>(k_fontWidth * k_fontHeight)); //Size of font texture is 1 byte hence just need width * height

	//upload to GPU - start
	vk::raii::CommandBuffer copyCommands = std::move(pDevice->CreateCommandBuffers(graphicsCommandPool, 1).front());
	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	copyCommands.begin(beginInfo);

	//Create image barrier to perform layout transition if needed
	vk::ImageMemoryBarrier preCopyBarrier = pDevice->CreateImageTransition(vk::AccessFlagBits::eNone,
		vk::AccessFlagBits::eTransferWrite,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eTransferDstOptimal,
		*textImage.image);

	copyCommands.pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eTransfer,
		vk::DependencyFlagBits::eByRegion,
		nullptr,
		nullptr,
		preCopyBarrier
	);

	//Do actual upload
	vk::BufferImageCopy copyRegion(
		0 /*offset*/,
		0 /*buffer row length*/,
		0 /*buffer image height*/,
		vk::ImageSubresourceLayers(
			vk::ImageAspectFlagBits::eColor,
			0/* mip level*/,
			0/* base array layer*/,
			1/* layer count*/
		),
		vk::Offset3D(0, 0, 0),
		vk::Extent3D(k_fontWidth, k_fontHeight, 1)
	);

	copyCommands.copyBufferToImage(
		*stagingBuffer.m_buffer,
		*textImage.image,
		vk::ImageLayout::eTransferDstOptimal,
		copyRegion
	);

	vk::ImageMemoryBarrier postCopyMemoryBarrier = pDevice->CreateImageTransition(
		vk::AccessFlagBits::eTransferWrite,
		vk::AccessFlagBits::eShaderRead,
		//Transfer layout to shader read only
		vk::ImageLayout::eTransferDstOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		*textImage.image
	);
	copyCommands.pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eFragmentShader,
		vk::DependencyFlagBits::eByRegion,
		nullptr,
		nullptr,
		postCopyMemoryBarrier
	);

	copyCommands.end();

	vk::Queue uploadQueue = pDevice->GetGraphicsQueue();
	vk::SubmitInfo submitInfo(nullptr, nullptr, *copyCommands, nullptr);
	uploadQueue.submit(submitInfo);

	//TODO better synchronization rather than waiting idle
	uploadQueue.waitIdle();

	//Create sampler for text image
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
	sampler = vk::raii::Sampler(pDevice->GetDevice(), samplerCreateInfo);

	//Font descriptor
	vk::DescriptorPoolSize poolSize(vk::DescriptorType::eCombinedImageSampler, 1);
	vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo(
		vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		1 /* max sets*/,
		poolSize
	);
	descriptorPool = vk::raii::DescriptorPool(pDevice->GetDevice(), descriptorPoolCreateInfo);

	vk::DescriptorSetLayoutBinding dslBinding
	(
		0 /*binding id*/,
		vk::DescriptorType::eCombinedImageSampler,
		vk::ShaderStageFlagBits::eFragment,
		*sampler
	);
	vk::DescriptorSetLayoutCreateInfo dslCreateInfo(
		{},
		dslBinding
	);
	overlayDescriptorLayout = vk::raii::DescriptorSetLayout(pDevice->GetDevice(), dslCreateInfo);

	vk::PipelineLayoutCreateInfo layoutCreateInfo({}, *overlayDescriptorLayout);
	overlayLayout = vk::raii::PipelineLayout(pDevice->GetDevice(), layoutCreateInfo);

	vk::DescriptorSetAllocateInfo allocInfo(*descriptorPool, *overlayDescriptorLayout);
	overlaySet = std::move(pDevice->GetDevice().allocateDescriptorSets(allocInfo).front());

	vk::DescriptorImageInfo textDescriptor(*sampler, *textImage.view, vk::ImageLayout::eShaderReadOnlyOptimal);

	vk::WriteDescriptorSet writeSet(*overlaySet, 0, 0, vk::DescriptorType::eCombinedImageSampler, textDescriptor);
	pDevice->GetDevice().updateDescriptorSets(writeSet, nullptr);

	overlayPipeline = CreateOverlayPipeline(pDevice, viewport, scissor, textVertShader, textFragShader, *overlayLayout);

	UpdateTextOverlay(*pDevice->GetDevice(), scissor.extent);
}

void GfxTextOverlay::RenderTextOverlay(GfxFrame const& frame, vk::Rect2D renderArea, vk::CommandBuffer const& commands)
{
	vk::CommandBufferBeginInfo const cbBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	commands.begin(cbBeginInfo);

	vk::ClearValue clearValues[2] =
	{
		vk::ClearValue(), //TODO why was one of these uninitialized?
		vk::ClearColorValue()
	};

	vk::RenderPassBeginInfo const beginInfo(*overlayRenderPass, *frame.frameBuffer, renderArea, clearValues);
	commands.beginRenderPass(beginInfo, vk::SubpassContents::eInline);
	commands.bindPipeline(vk::PipelineBindPoint::eGraphics, *overlayPipeline);
	commands.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *overlayLayout, 0, *overlaySet, nullptr);

	commands.bindVertexBuffers(0, *overlayVertexBuffer.m_buffer, { 0 });

	for (uint32_t i = 0; i < overlayText.size(); ++i)
	{
		commands.draw(4, 1, i * 4, 0);
	}

	commands.endRenderPass();
	commands.end();

}

void GfxTextOverlay::UpdateTextOverlay(vk::Device device, vk::Extent2D frameBufferDim)
{
	//fill in vertex buffer with the correct uv data
	TextVertex* pIter = (TextVertex*)overlayVertexBuffer.m_pData;

	if (!pIter)
	{
		throw InitializationException("Cannot map text memory");
	}

	//Add text
	float x = (5.0f / (float)frameBufferDim.width * 2.0) - 1.0f;
	float y = (5.0f / (float)frameBufferDim.height * 2.0) - 1.0f;

	uint32_t const firstChar = STB_FONT_consolas_24_latin1_FIRST_CHAR;

	float const charW = 2.5f / frameBufferDim.width;
	float const charH = 2.5f / frameBufferDim.height;

	//Calculate total text width
	float textWidth = 0;
	for (char letter : overlayText)
	{
		stb_fontchar* charData = &stbFontData[(uint32_t)letter - firstChar];
		textWidth += charData->advance * charW;
	}

	//Generate uv mapped quad per char
	//TODO proper ranges for mapped memory and iterating through it?
	for (char letter : overlayText)
	{
		stb_fontchar* charData = &stbFontData[(uint32_t)letter - firstChar];

		//Top left
		pIter->vx = x + charData->x0 * charW;
		pIter->vy = -(y + charData->y0 * charH);
		pIter->u = charData->s0f;
		pIter->v = charData->t0f;
		pIter++;

		//Bottom left
		pIter->vx = x + charData->x0 * charW;
		pIter->vy = -(y + charData->y1 * charH);
		pIter->u = charData->s0f;
		pIter->v = charData->t1f;
		pIter++;

		//Top right
		pIter->vx = x + charData->x1 * charW;
		pIter->vy = -(y + charData->y0 * charH);
		pIter->u = charData->s1f;
		pIter->v = charData->t0f;
		pIter++;

		//Bottom right
		pIter->vx = x + charData->x1 * charW;
		pIter->vy = -(y + charData->y1 * charH);
		pIter->u = charData->s1f;
		pIter->v = charData->t1f;
		pIter++;

		x += charData->advance * charW;

	}
}

vk::raii::RenderPass GfxTextOverlay::CreateOverlayRenderPass(GfxDevicePtr_t pDevice, vk::Format colorFormat, vk::Format depthFormat)
{
	std::array<vk::AttachmentDescription, 2> attachments =
	{
		//Color attachment
		vk::AttachmentDescription(
			{},
			colorFormat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eLoad, //Dont clear as this is an overlay
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::ePresentSrcKHR,
			vk::ImageLayout::ePresentSrcKHR
		),
		//Depth attachment
		vk::AttachmentDescription(
			{},
			depthFormat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthStencilAttachmentOptimal
		)
	};

	std::array<vk::SubpassDependency, 2> dependencies = {
		//Transition from final to initial
		vk::SubpassDependency(
			VK_SUBPASS_EXTERNAL,
			0,
			vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::AccessFlagBits::eMemoryRead,
			vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
			vk::DependencyFlagBits::eByRegion
		),
		//Transition from initial to final
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

	return std::move(GfxPipelineBuilder::CreateRenderPass(pDevice->GetDevice(), attachments, dependencies));
}

vk::raii::Pipeline GfxTextOverlay::CreateOverlayPipeline(GfxDevicePtr_t pDevice, vk::Viewport viewport, vk::Rect2D scissor, vk::ShaderModule textVertShader, vk::ShaderModule textFragShader, vk::PipelineLayout pipelineLayout)
{
	GfxPipelineBuilder builder;
	vk::PipelineColorBlendAttachmentState colorBlend(
		VK_TRUE,
		vk::BlendFactor::eSrcAlpha,
		vk::BlendFactor::eOneMinusSrcAlpha,
		vk::BlendOp::eAdd,
		vk::BlendFactor::eOneMinusSrcAlpha,
		vk::BlendFactor::eZero,
		vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
	);

	builder._inputAssembly = GfxPipelineBuilder::CreateInputAssemblyInfo(vk::PrimitiveTopology::eTriangleStrip);
	builder._rasterizer = GfxPipelineBuilder::CreateRasterizationStateInfo(vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone);
	builder._colorBlendAttachment = colorBlend;
	builder._depthStencil = GfxPipelineBuilder::CreateDepthStencilStateInfo(VK_TRUE, VK_TRUE, vk::CompareOp::eLessOrEqual);
	builder._viewport = viewport;
	builder._scissor = scissor;
	builder._multisampling = GfxPipelineBuilder::CreateMultisampleStateInfo();
	builder._vertexDescription = TextVertex::GetDescription();
	builder._shaderStages.push_back(
		GfxPipelineBuilder::CreateShaderStageInfo(vk::ShaderStageFlagBits::eVertex, textVertShader)
	);
	builder._shaderStages.push_back(
		GfxPipelineBuilder::CreateShaderStageInfo(vk::ShaderStageFlagBits::eFragment, textFragShader)
	);
	builder._pipelineLayout = pipelineLayout;
	overlayRenderPass = CreateOverlayRenderPass(pDevice, vk::Format::eB8G8R8A8Unorm, vk::Format::eD16Unorm);

	return builder.BuildPipeline(pDevice->GetDevice(), *overlayRenderPass);
}
