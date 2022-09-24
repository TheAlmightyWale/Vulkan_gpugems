#pragma once
#include "GfxFwdDecl.h"
#include <cstdint>
#include <unordered_map>

enum class DataUsageFrequency {
	ePerFrame,
	ePerModel,
};

struct DescriptorInfo {
	DescriptorInfo() : set(nullptr), layout(nullptr) {}
	vk::raii::DescriptorSet set;
	vk::raii::DescriptorSetLayout layout;
};

using DescriptorSlotMap = std::unordered_map<DataUsageFrequency, DescriptorInfo>;

uint32_t const k_MaxDescriptorsToAllocate = 1000; /* arbitrary*/

//Descriptor manager holds the descriptor pools for the engine as well as a set of pre-defined descriptorSets
// which other components can specify they want to add things to
class GfxDescriptorManager
{
public:
	GfxDescriptorManager(GfxDevicePtr_t pDevice);

	void SetUniformBinding(uint32_t bindingId, vk::ShaderStageFlagBits bindToStage, DataUsageFrequency usageFrequency, bool bDynamic);

	vk::DescriptorSet GetDescriptor(DataUsageFrequency usageFrequency) const;
	vk::DescriptorSetLayout GetLayout(DataUsageFrequency usageFrequency) const;

private:
	DescriptorSlotMap m_descriptorSlots;
	vk::raii::DescriptorPool m_descriptorPool;
	GfxDevicePtr_t m_pGfxDevice;
};

using GfxDescriptorManagerPtr_t = std::unique_ptr<GfxDescriptorManager>;

