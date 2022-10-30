#include "GfxDescriptorManager.h"
#include "GfxDevice.h"
#include "Logger.h"

#include <algorithm>

GfxDescriptorManager::GfxDescriptorManager(GfxDevicePtr_t pDevice)
	: m_descriptorPool(nullptr)
	, m_descriptorSlots()
	, m_pGfxDevice(pDevice)
{

	SPDLOG_INFO("Initializing Descriptor Manager");

	//TODO do some enum reflection so we can just iterate over a class rather than manually filling this out?
	std::array<DataUsageFrequency, 3> usageFrequencies = {
		DataUsageFrequency::ePerFrame,
		DataUsageFrequency::ePerModel,
		DataUsageFrequency::ePerMaterial,
	};

	//Just uniform buffers for now
	std::vector<vk::DescriptorPoolSize>poolSizes = {
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, k_MaxDescriptorsToAllocate),
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, k_MaxDescriptorsToAllocate),
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, k_MaxDescriptorsToAllocate),
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, k_MaxDescriptorsToAllocate)
	};

	vk::DescriptorPoolCreateInfo poolCreateInfo(
		{vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet},
		usageFrequencies.size(),
		poolSizes
	);

	m_descriptorPool = vk::raii::DescriptorPool(m_pGfxDevice->GetDevice(), poolCreateInfo);

	//Pre-initialize all descriptor slots
	for (DataUsageFrequency freq : usageFrequencies)
	{
		m_descriptorSlots.emplace(std::make_pair(freq, DescriptorInfo()));
	}

}

void GfxDescriptorManager::AddBinding(uint32_t bindingId, vk::ShaderStageFlagBits bindToStage, DataUsageFrequency usageFrequency, vk::DescriptorType type)
{
	vk::DescriptorSetLayoutBinding dslBinding(
		bindingId,
		type,
		1 /*assuming 1 descriptor, no arrays*/,
		bindToStage,
		nullptr
	);

	DescriptorInfo& info = m_descriptorSlots.at(usageFrequency);
	info.bindings.push_back(dslBinding);

	//Sort every time for now
	std::sort(info.bindings.begin(), info.bindings.end(),
		[](vk::DescriptorSetLayoutBinding& a, vk::DescriptorSetLayoutBinding& b)
		{
			return a.binding < b.binding;
		});

	//Re-create set and layout every time for now
	//multiple bindings per descriptorSetLayout
	vk::DescriptorSetLayoutCreateInfo dslCreateInfo({}, info.bindings);
	info.layout = std::move(vk::raii::DescriptorSetLayout(m_pGfxDevice->GetDevice(), dslCreateInfo));

	//One layout per set, but we can allocate multiple sets at once
	vk::DescriptorSetAllocateInfo dsaInfo(*m_descriptorPool, *info.layout);
	info.set = std::move(m_pGfxDevice->GetDevice().allocateDescriptorSets(dsaInfo).front());
}

vk::DescriptorSet GfxDescriptorManager::GetDescriptor(DataUsageFrequency usageFrequency) const
{
	return *m_descriptorSlots.at(usageFrequency).set;
}

vk::DescriptorSetLayout GfxDescriptorManager::GetLayout(DataUsageFrequency usageFrequency) const
{
	return *m_descriptorSlots.at(usageFrequency).layout;
}

DescriptorInfo const*  GfxDescriptorManager::GetDescriptorInfo(DataUsageFrequency usageFrequency) const
{
	return &m_descriptorSlots.at(usageFrequency);
}

vk::WriteDescriptorSet GfxDescriptorManager::GetWriteDescriptor(DataUsageFrequency usageFrequency, uint32_t bindingId) const
{
	DescriptorInfo const& info = m_descriptorSlots.at(usageFrequency);
	vk::DescriptorSetLayoutBinding const& binding = info.bindings.at(bindingId);

	vk::WriteDescriptorSet writeDescriptor(
		*info.set,
		binding.binding,
		0,
		binding.descriptorType,
		nullptr, nullptr, nullptr, nullptr //Undefined, for the caller to fill out for thier purposes
	);

	return writeDescriptor;
}
