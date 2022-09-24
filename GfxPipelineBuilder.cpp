#include "GfxPipelineBuilder.h"
//TODO split out mesh and vertex defs
#include "Mesh.h"
#include "Logger.h"

vk::raii::Pipeline GfxPipelineBuilder::BuildPipeline(vk::raii::Device const& device, vk::RenderPass renderPass)
{
    SPDLOG_INFO("Building Pipeline");

    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo({}, _viewport, _scissor);
    vk::PipelineColorBlendStateCreateInfo colorBlending(
        {},
        VK_FALSE, //Logic Op enable
        vk::LogicOp::eCopy, //Logic op
        _colorBlendAttachment);

    vk::PipelineVertexInputStateCreateInfo vertexInput({}, _vertexDescription.bindings, _vertexDescription.attributes);

    vk::GraphicsPipelineCreateInfo pipelineInfo(
        {},
        _shaderStages,
        &vertexInput,
        &_inputAssembly,
        nullptr /*tesslation state*/,
        &viewportStateCreateInfo,
        &_rasterizer,
        &_multisampling,
        &_depthStencil /* depth stencil state*/,
        &colorBlending,
        nullptr /*dynamic state*/,
        _pipelineLayout,
        renderPass,
        0 /*subpass*/,
        VK_NULL_HANDLE,
        0 /*base pipeline index*/
    );

    vk::raii::Pipeline pipeline(
        device,
        nullptr,
        pipelineInfo);

    return std::move(pipeline);

}

vk::PipelineShaderStageCreateInfo GfxPipelineBuilder::CreateShaderStageInfo(vk::ShaderStageFlagBits stage, vk::ShaderModule shaderModule)
{
    vk::PipelineShaderStageCreateInfo createInfo(
        {},
        stage,
        shaderModule,
        "main" //assume main for now
    );
       
    return createInfo;
}

vk::PipelineInputAssemblyStateCreateInfo GfxPipelineBuilder::CreateInputAssemblyInfo(vk::PrimitiveTopology topology)
{
    vk::PipelineInputAssemblyStateCreateInfo createInfo({}, topology, VK_FALSE);

    return createInfo;
}

vk::PipelineRasterizationStateCreateInfo GfxPipelineBuilder::CreateRasterizationStateInfo(vk::PolygonMode polygonMode, vk::CullModeFlagBits cullMode)
{
    vk::PipelineRasterizationStateCreateInfo createInfo(
        {},
        VK_FALSE, //Depth Clamp enable
        VK_FALSE, //Rasterizer discard enabled
        polygonMode,
        cullMode,
        vk::FrontFace::eClockwise,
        VK_FALSE, //Depth bias enable
        0.0f, //DepthBias constant
        0.0f, //depth bias clamp
        0.0f, //Depth bias slope
        1.0f); //line width

    return createInfo;
}

vk::PipelineMultisampleStateCreateInfo GfxPipelineBuilder::CreateMultisampleStateInfo()
{
    vk::PipelineMultisampleStateCreateInfo createInfo;
    createInfo.setSampleShadingEnable(VK_FALSE);
    createInfo.setRasterizationSamples(vk::SampleCountFlagBits::e1); //No multisampling
    createInfo.setMinSampleShading(1.0f);
    createInfo.setPSampleMask(nullptr);
    createInfo.setAlphaToCoverageEnable(VK_FALSE);
    createInfo.setAlphaToOneEnable(VK_FALSE);

    return createInfo;
}

vk::PipelineColorBlendAttachmentState GfxPipelineBuilder::CreateColorBlendAttachmentState()
{
    vk::PipelineColorBlendAttachmentState createInfo;
    createInfo.setColorWriteMask(
        vk::ColorComponentFlagBits::eR
        | vk::ColorComponentFlagBits::eG
        | vk::ColorComponentFlagBits::eB
        | vk::ColorComponentFlagBits::eA);

    createInfo.setBlendEnable(VK_FALSE);

    return createInfo;
}

vk::raii::PipelineLayout GfxPipelineBuilder::CreatePipelineLayout(
    vk::raii::Device const& device,
    vk::ArrayProxyNoTemporaries<vk::PushConstantRange> pushConstants,
    vk::ArrayProxyNoTemporaries<vk::DescriptorSetLayout> descriptorSets)
{
    vk::PipelineLayoutCreateInfo createInfo({}/*flags*/, descriptorSets, pushConstants);
    vk::raii::PipelineLayout layout(device, createInfo);

    return std::move(layout);
}

//TODO assumes you have 2 attachments first is color second is depth stencil
vk::raii::RenderPass GfxPipelineBuilder::CreateRenderPass(
    vk::raii::Device const& device,
    vk::ArrayProxyNoTemporaries<vk::AttachmentDescription const> const& attachments,
    vk::ArrayProxyNoTemporaries<vk::SubpassDependency const> const& dependencies
)
{
    //TODO bundle attachment description and references together rather than hardcode here
    vk::AttachmentReference colorReference(0/*attachment*/, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depthReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    vk::SubpassDescription subpass({} /*flags*/, vk::PipelineBindPoint::eGraphics, {}/*input attachments*/, colorReference, {}/*resolve attachments*/, &depthReference);

    vk::RenderPassCreateInfo renderPassCreateInfo({}, attachments, subpass, dependencies);
    return std::move(vk::raii::RenderPass(device, renderPassCreateInfo));
}

vk::PipelineDepthStencilStateCreateInfo GfxPipelineBuilder::CreateDepthStencilStateInfo(vk::Bool32 enableTest, vk::Bool32 enableWrite, vk::CompareOp compareOp)
{
    vk::PipelineDepthStencilStateCreateInfo createInfo(
        {},
        enableTest /*enable test*/,
        enableWrite /*enable writes to depth buffer*/,
        compareOp /*comparision of fragment to already written value*/,
        VK_FALSE /*bounds test*/,
        VK_FALSE /*stencil test*/,
        vk::StencilOp::eZero /*unused stencil ops*/,
        vk::StencilOp::eZero,
        0.0f /*min depth bound*/,
        1.0f /*max depth bound*/);
    return createInfo;
}
