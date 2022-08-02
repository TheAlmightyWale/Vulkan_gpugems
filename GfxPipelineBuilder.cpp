#include "GfxPipelineBuilder.h"
#include "Mesh.h"

vk::raii::Pipeline GfxPipelineBuilder::BuildPipeline(vk::raii::Device const& device, vk::RenderPass renderPass)
{
    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo({}, _viewport, _scissor);
    vk::PipelineColorBlendStateCreateInfo colorBlending(
        {},
        VK_FALSE, //Logic Op enable
        vk::LogicOp::eCopy, //Logic op
        _colorBlendAttachment);

    vk::VertexInputBindingDescription description(0, sizeof(Vertex)); //Vertex should be 4 * 6 = 24bytes
    std::array<vk::VertexInputAttributeDescription, 2> attributes =
    {
        vk::VertexInputAttributeDescription(0 /*location*/, 0 /*binding*/, vk::Format::eR32G32B32Sfloat, 0/*offset*/),
        vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, 12 /*3 floats from position*/)
    };
    vk::PipelineVertexInputStateCreateInfo createInfo({}, description, attributes);
    _vertexInputInfo = createInfo;

    vk::GraphicsPipelineCreateInfo pipelineInfo(
        {},
        _shaderStages,
        &_vertexInputInfo,
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

//TODO move to shader based vertex attributes?
vk::PipelineVertexInputStateCreateInfo GfxPipelineBuilder::CreateVertexInputStateInfo()
{
    vk::VertexInputBindingDescription description(0, sizeof(Vertex)); //Vertex should be 4 * 6 = 24bytes
    std::array<vk::VertexInputAttributeDescription, 2> attributes =
    {
        vk::VertexInputAttributeDescription(0 /*location*/, 0 /*binding*/, vk::Format::eR32G32B32Sfloat, 0/*offset*/),
        vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, 12 /*3 floats from position*/)
    };
    vk::PipelineVertexInputStateCreateInfo createInfo({}, description, attributes);

    return createInfo;
}

vk::PipelineInputAssemblyStateCreateInfo GfxPipelineBuilder::CreateInputAssemblyInfo(vk::PrimitiveTopology topology)
{
    vk::PipelineInputAssemblyStateCreateInfo createInfo({}, topology, VK_FALSE);

    return createInfo;
}

vk::PipelineRasterizationStateCreateInfo GfxPipelineBuilder::CreateRasterizationStateInfo(vk::PolygonMode polygonMode)
{
    vk::PipelineRasterizationStateCreateInfo createInfo(
        {},
        VK_FALSE, //Depth Clamp enable
        VK_FALSE, //Rasterizer discard enabled
        polygonMode,
        vk::CullModeFlagBits::eBack,
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

vk::raii::PipelineLayout GfxPipelineBuilder::CreatePipelineLayout(vk::raii::Device const& device, vk::PushConstantRange pushConstant)
{
    vk::PipelineLayoutCreateInfo createInfo({}/*flags*/, {}/*descriptor sets*/, pushConstant);
    vk::raii::PipelineLayout layout(device, createInfo);

    return std::move(layout);
}

vk::raii::RenderPass GfxPipelineBuilder::CreateRenderPass(
    vk::raii::Device const& device,
    vk::Format renderSurfaceFormat,
    vk::Format depthSurfaceFormat,
    vk::ArrayProxyNoTemporaries<vk::AttachmentDescription const> const& attachments
)
{
    //TODO bundle attachment description and references together rather than hardcode here
    vk::AttachmentReference colorReference(0/*attachment*/, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depthReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    vk::SubpassDescription subpass({} /*flags*/, vk::PipelineBindPoint::eGraphics, {}/*input attachments*/, colorReference, {}/*resolve attachments*/, &depthReference);

    vk::SubpassDependency colorAttachmentDependency(
        VK_SUBPASS_EXTERNAL,
        0 /*dst subpass*/,
        vk::PipelineStageFlagBits::eColorAttachmentOutput/*srcStage*/,
        vk::PipelineStageFlagBits::eColorAttachmentOutput/*dstStage*/,
        vk::AccessFlagBits::eNone /*src access mask*/,
        vk::AccessFlagBits::eColorAttachmentWrite,
        {} /*dependency flags*/
    );

    vk::SubpassDependency depthDependency(
        VK_SUBPASS_EXTERNAL,
        0,
        vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
        vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
        vk::AccessFlagBits::eNone,
        vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        {}
    );

    std::array<vk::SubpassDependency, 2> dependencies;
    dependencies[0] = colorAttachmentDependency;
    dependencies[1] = depthDependency;

    vk::RenderPassCreateInfo renderPassCreateInfo({}, attachments, subpass, dependencies);
    return std::move(vk::raii::RenderPass(device, renderPassCreateInfo));
}

vk::PipelineDepthStencilStateCreateInfo GfxPipelineBuilder::CreateDepthStencilStateInfo()
{
    vk::PipelineDepthStencilStateCreateInfo createInfo(
        {},
        VK_TRUE /*enable test*/,
        VK_TRUE /*enable writes to depth buffer*/,
        vk::CompareOp::eLess /*comparision of fragment to already written value*/,
        VK_FALSE /*bounds test*/,
        VK_FALSE /*stencil test*/,
        vk::StencilOp::eZero /*unused stencil ops*/,
        vk::StencilOp::eZero,
        0.0f /*min depth bound*/,
        1.0f /*max depth bound*/);
    return createInfo;
}
