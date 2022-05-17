#include "GfxPipelineBuilder.h"
#include "GfxUniformBuffer.h"

vk::raii::Pipeline GfxPipelineBuilder::BuildPipeline(vk::raii::Device const& device, vk::RenderPass renderPass)
{
    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo({}, _viewport, _scissor);
    vk::PipelineColorBlendStateCreateInfo colorBlending(
        {},
        VK_FALSE, //Logic Op enable
        vk::LogicOp::eCopy, //Logic op
        _colorBlendAttachment);

    vk::GraphicsPipelineCreateInfo pipelineInfo(
        {},
        _shaderStages,
        &_vertexInputInfo,
        &_inputAssembly,
        nullptr /*tesslation state*/,
        &viewportStateCreateInfo,
        &_rasterizer,
        &_multisampling,
        nullptr /* depth stencil state*/,
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

vk::PipelineVertexInputStateCreateInfo GfxPipelineBuilder::CreateVertexInputStateInfo()
{
    vk::PipelineVertexInputStateCreateInfo createInfo({}, nullptr, nullptr);

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
        VK_FALSE, //Rasterizer disacard enabled
        polygonMode,
        vk::CullModeFlagBits::eNone,
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

vk::raii::PipelineLayout GfxPipelineBuilder::CreatePipelineLayout(vk::raii::Device const& device)
{
    //Empty layout for now
    vk::PipelineLayoutCreateInfo createInfo;
    vk::raii::PipelineLayout layout(device, createInfo);

    return std::move(layout);
}

vk::raii::RenderPass GfxPipelineBuilder::CreateRenderPass(
    vk::raii::Device const& device,
    GfxUniformBuffer const& descriptorBuffer,
    vk::Format renderSurfaceFormat,
    vk::Format depthSurfaceFormat,
    vk::ArrayProxyNoTemporaries<vk::AttachmentDescription const> const& attachments
)
{
    //vk::DescriptorType descriptor = vk::DescriptorType::eUniformBuffer;
    //uint32_t descriptorCount = 1;

    //vk::DescriptorSetLayoutBinding layoutBinding(0 /*binding*/, descriptor, descriptorCount, vk::ShaderStageFlagBits::eVertex);
    //vk::DescriptorSetLayoutCreateInfo layoutCreateInfo({}/*flags*/, layoutBinding);
    //vk::raii::DescriptorSetLayout layout(device, layoutCreateInfo);

    //vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo({}, *layout);
    //vk::raii::PipelineLayout pipelineLayout(device, pipelineLayoutCreateInfo);

    //vk::DescriptorPoolSize poolSize(descriptor, descriptorCount);
    //vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1 /*maxSets*/, poolSize);
    //vk::raii::DescriptorPool descriptorPool(device, descriptorPoolCreateInfo);

    //vk::DescriptorSetAllocateInfo allocateInfo(*descriptorPool, *layout);
    //vk::raii::DescriptorSet descriptorSet = std::move(vk::raii::DescriptorSets(device, allocateInfo).front());

    ////TODO move writing/binding of descriptorsets into pipeline object
    //vk::DescriptorBufferInfo uniformBuffer(*descriptorBuffer.buffer, 0/*offset*/, descriptorBuffer.buffer.getMemoryRequirements().size);
    //vk::WriteDescriptorSet descriptorWrite(*descriptorSet, 0/*binding*/, 1 /*array element*/, descriptor, nullptr, uniformBuffer);
    ////m_pDevice->updateDescriptorSets(descriptorWrite, nullptr);

    //TODO bundle attachment description and references together rather than hardcode here
    vk::AttachmentReference colorReference(0/*attachment*/, vk::ImageLayout::eColorAttachmentOptimal);
    //vk::AttachmentReference depthReference(1, vk::ImageLayout::eDepthAttachmentOptimal);
    //TODO: add depth
    vk::SubpassDescription subpass({} /*flags*/, vk::PipelineBindPoint::eGraphics, {}/*input attachments*/, colorReference, {}/*resolve attachments*/, nullptr/*&depthReference*/);

    vk::RenderPassCreateInfo renderPassCreateInfo({}, attachments, subpass);
    return std::move(vk::raii::RenderPass(device, renderPassCreateInfo));
}
