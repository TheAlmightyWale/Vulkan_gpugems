#pragma once
#include <vector>
#include "GfxFwdDecl.h"

class GfxPipelineBuilder
{
public:
	std::vector<vk::PipelineShaderStageCreateInfo> _shaderStages;
	VertexDescription _vertexDescription;
	vk::PipelineInputAssemblyStateCreateInfo _inputAssembly;
	vk::Viewport _viewport;
	vk::Rect2D _scissor;
	vk::PipelineRasterizationStateCreateInfo _rasterizer;
	vk::PipelineColorBlendAttachmentState _colorBlendAttachment;
	vk::PipelineMultisampleStateCreateInfo _multisampling;
	vk::PipelineDepthStencilStateCreateInfo _depthStencil;
	vk::PipelineLayout _pipelineLayout;

	vk::raii::Pipeline BuildPipeline(vk::raii::Device const& device, vk::RenderPass pass);

	static vk::PipelineShaderStageCreateInfo CreateShaderStageInfo(vk::ShaderStageFlagBits stage, vk::ShaderModule shaderModule);
	static vk::PipelineInputAssemblyStateCreateInfo CreateInputAssemblyInfo(vk::PrimitiveTopology topology);
	static vk::PipelineRasterizationStateCreateInfo CreateRasterizationStateInfo(vk::PolygonMode polygonMode, vk::CullModeFlagBits cullMode);
	static vk::PipelineMultisampleStateCreateInfo CreateMultisampleStateInfo();
	static vk::PipelineColorBlendAttachmentState CreateColorBlendAttachmentState();
	static vk::PipelineDepthStencilStateCreateInfo CreateDepthStencilStateInfo(vk::Bool32 enableTest, vk::Bool32 enableWrite, vk::CompareOp compareOp);
	static vk::raii::RenderPass CreateRenderPass(
		vk::raii::Device const& device,
		vk::ArrayProxyNoTemporaries<vk::AttachmentDescription const> const& attachments,
		vk::ArrayProxyNoTemporaries<vk::SubpassDependency const> const& dependencies
	);

	static vk::raii::PipelineLayout CreatePipelineLayout(
		vk::raii::Device const& device,
		vk::ArrayProxyNoTemporaries<vk::PushConstantRange> pushConstants,
		vk::ArrayProxyNoTemporaries<vk::DescriptorSetLayout> descriptorSets);

};

