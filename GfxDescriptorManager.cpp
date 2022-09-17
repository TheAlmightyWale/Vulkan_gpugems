#include "GfxDescriptorManager.h"
#include "GfxDevice.h"
#include "Logger.h"

GfxDescriptorManager::GfxDescriptorManager(GfxDevicePtr_t pDevice, uint64_t minBufferAlignmentBytes)
	: m_descriptorSlots()
	, m_descriptorPool(nullptr)
	, m_pGfxDevice(pDevice)
{

	SPDLOG_INFO("Initializing Descriptor Manager. Minimum GPU buffer alignment is: {0} bytes", minBufferAlignmentBytes);

	//TODO do some enum reflection so we can just iterate over a class rather than manually filling this out?
	std::array<DataUsageFrequency, 2> usageFrequencies = {
		DataUsageFrequency::ePerFrame,
		DataUsageFrequency::ePerModel
	};

	//Just uniform buffers for now
	vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer, k_MaxDescriptorsToAllocate);
	vk::DescriptorPoolCreateInfo poolCreateInfo(
		{},
		usageFrequencies.size(),
		poolSize
	);

	m_descriptorPool = vk::raii::DescriptorPool(m_pGfxDevice->GetDevice(), poolCreateInfo);
}

void GfxDescriptorManager::SetUniformBinding(uint32_t bindingId, vk::ShaderStageFlagBits bindToStage, DataUsageFrequency usageFrequency)
{
	DescriptorInfo info;

	vk::DescriptorSetLayoutBinding dslBinding(
		bindingId,
		vk::DescriptorType::eUniformBuffer,
		1 /*assuming 1 descriptor, no arrays*/,
		bindToStage,
		nullptr
	);

	//multiple bindings per descriptorSetLayout
	vk::DescriptorSetLayoutCreateInfo dslCreateInfo({}, dslBinding);
	info.layout = std::move(vk::raii::DescriptorSetLayout(m_pGfxDevice->GetDevice(), dslCreateInfo));
	//Multiple layouts per set
	vk::DescriptorSetAllocateInfo dsaInfo(*m_descriptorPool, *info.layout);
	info.set = std::move(m_pGfxDevice->GetDevice().allocateDescriptorSets(dsaInfo).front());
	
	if (m_descriptorSlots.contains(usageFrequency))
	{
		m_descriptorSlots.at(usageFrequency) = std::move(info);
	}
	else
	{
		m_descriptorSlots.emplace(std::make_pair(usageFrequency, std::move(info)));
	}
}

vk::DescriptorSet GfxDescriptorManager::GetDescriptor(DataUsageFrequency usageFrequency) const
{
	return *m_descriptorSlots.at(usageFrequency).set;
}

vk::DescriptorSetLayout GfxDescriptorManager::GetLayout(DataUsageFrequency usageFrequency) const
{
	return *m_descriptorSlots.at(usageFrequency).layout;
}
