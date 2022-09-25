#include "GfxDescriptorManager.h"
#include "GfxDevice.h"
#include "Logger.h"

GfxDescriptorManager::GfxDescriptorManager(GfxDevicePtr_t pDevice)
	: m_descriptorSlots()
	, m_descriptorPool(nullptr)
	, m_pGfxDevice(pDevice)
{

	SPDLOG_INFO("Initializing Descriptor Manager");

	//TODO do some enum reflection so we can just iterate over a class rather than manually filling this out?
	std::array<DataUsageFrequency, 2> usageFrequencies = {
		DataUsageFrequency::ePerFrame,
		DataUsageFrequency::ePerModel
	};

	//Just uniform buffers for now
	std::vector<vk::DescriptorPoolSize>poolSizes = {
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, k_MaxDescriptorsToAllocate),
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, k_MaxDescriptorsToAllocate),
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, k_MaxDescriptorsToAllocate)
	};

	vk::DescriptorPoolCreateInfo poolCreateInfo(
		{},
		usageFrequencies.size(),
		poolSizes
	);

	m_descriptorPool = vk::raii::DescriptorPool(m_pGfxDevice->GetDevice(), poolCreateInfo);
}

//TODO descriptorManager should be paired with a descriptor builder, which you add desired bindings before inputting that to a pipeline
	// descriptorManager should probably just be interested in managing updates to descriptorSet information, such as getting locations of certain bindings etc
void GfxDescriptorManager::SetBinding(uint32_t bindingId, vk::ShaderStageFlagBits bindToStage, DataUsageFrequency usageFrequency, vk::DescriptorType type)
{
	DescriptorInfo info;

	vk::DescriptorSetLayoutBinding dslBinding(
		bindingId,
		type,
		1 /*assuming 1 descriptor, no arrays*/,
		bindToStage,
		nullptr
	);

	//multiple bindings per descriptorSetLayout
	vk::DescriptorSetLayoutCreateInfo dslCreateInfo({}, dslBinding);
	info.layout = std::move(vk::raii::DescriptorSetLayout(m_pGfxDevice->GetDevice(), dslCreateInfo));
	//One layout per set, but we can allocate multiple sets at once
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
