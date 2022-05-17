#pragma once
#include <vector>
#include "GfxFwdDecl.h"

class GfxPipelineBuilder
{
public:
	std::vector<vk::PipelineShaderStageCreateInfo> _shaderStages;
	vk::PipelineVertexInputStateCreateInfo _vertexInputInfo;
	vk::PipelineInputAssemblyStateCreateInfo _inputAssembly;
	vk::Viewport _viewport;
	vk::Rect2D _scissor;
	vk::PipelineRasterizationStateCreateInfo _rasterizer;
	vk::PipelineColorBlendAttachmentState _colorBlendAttachment;
	vk::PipelineMultisampleStateCreateInfo _multisampling;
	vk::PipelineLayout _pipelineLayout;

	vk::raii::Pipeline BuildPipeline(vk::raii::Device const& device, vk::RenderPass pass);

	static vk::PipelineShaderStageCreateInfo CreateShaderStageInfo(vk::ShaderStageFlagBits stage, vk::ShaderModule shaderModule);
	static vk::PipelineVertexInputStateCreateInfo CreateVertexInputStateInfo();
	static vk::PipelineInputAssemblyStateCreateInfo CreateInputAssemblyInfo(vk::PrimitiveTopology topology);
	static vk::PipelineRasterizationStateCreateInfo CreateRasterizationStateInfo(vk::PolygonMode polygonMode);
	static vk::PipelineMultisampleStateCreateInfo CreateMultisampleStateInfo();
	static vk::PipelineColorBlendAttachmentState CreateColorBlendAttachmentState();
	static vk::raii::RenderPass CreateRenderPass(
		vk::raii::Device const& device,
		GfxUniformBuffer const& descriptorBuffer,
		vk::Format renderSurfaceFormat,
		vk::Format depthSurfaceFormat,
		vk::ArrayProxyNoTemporaries<vk::AttachmentDescription const> const& attachments
	);
	static vk::raii::PipelineLayout CreatePipelineLayout(vk::raii::Device const& device);

};

