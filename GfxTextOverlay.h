#pragma once
#include "GfxFwdDecl.h"
#include "stb_font_consolas_24_latin1.inl"
#include "GfxImage.h"
#include "GfxBuffer.h"

constexpr uint32_t k_max_char_count = 2048;
std::string const overlayText = "hello there";

class GfxTextOverlay
{
public:
	GfxTextOverlay();

	GfxTextOverlay(
		GfxDevicePtr_t pDevice,
		vk::CommandPool graphicsCommandPool,
		vk::Viewport viewport,
		vk::Rect2D scissor);

	void RenderTextOverlay(GfxFrame const& frame, vk::Rect2D renderArea, vk::CommandBuffer const& commands);

private:
	void UpdateTextOverlay(vk::Device device, vk::Extent2D frameBufferDim);
	vk::raii::RenderPass CreateOverlayRenderPass(GfxDevicePtr_t pDevice, vk::Format colorFormat, vk::Format depthFormat);
	vk::raii::Pipeline CreateOverlayPipeline(
		GfxDevicePtr_t pDevice,
		vk::Viewport viewport,
		vk::Rect2D scissor,
		vk::ShaderModule textVertShader,
		vk::ShaderModule textFragShader,
		vk::PipelineLayout pipelineLayout);

	stb_fontchar stbFontData[STB_FONT_consolas_24_latin1_NUM_CHARS];
	vk::raii::CommandBuffers textOverlayCommandBuffers;
	GfxImage textImage;
	vk::raii::Sampler sampler;
	vk::raii::DescriptorPool descriptorPool;
	vk::raii::DescriptorSetLayout overlayDescriptorLayout;
	vk::raii::PipelineLayout overlayLayout;
	vk::raii::DescriptorSet overlaySet;
	vk::raii::RenderPass overlayRenderPass;
	vk::raii::Pipeline overlayPipeline;
	GfxBuffer overlayVertexBuffer;
	std::array<vk::raii::Framebuffer, 2> overlayFrameBuffers;

};

