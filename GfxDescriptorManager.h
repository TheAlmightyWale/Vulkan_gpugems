#pragma once
#include "GfxFwdDecl.h"
#include <cstdint>
#include <unordered_map>

enum class DataUsageFrequency {
	ePerFrame,
	ePerModel,
	ePerMaterial,
};

struct DescriptorInfo {
	DescriptorInfo() noexcept
		: set(nullptr)
		, layout(nullptr)
		, bindings()
	{}
	vk::raii::DescriptorSet set;
	vk::raii::DescriptorSetLayout layout;
	//Assume compact vector where position in array matches binding id
	std::vector<vk::DescriptorSetLayoutBinding> bindings;
};

using DescriptorSlotMap = std::unordered_map<DataUsageFrequency, DescriptorInfo>;

constexpr uint32_t k_MaxDescriptorsToAllocate = 100; /* arbitrary*/

//Descriptor manager holds the descriptor pools for the engine as well as a set of pre-defined descriptorSets
// which other components can specify they want to add things to
class GfxDescriptorManager
{
public:
	GfxDescriptorManager(GfxDevicePtr_t pDevice);

	void AddBinding(uint32_t bindingId, vk::ShaderStageFlagBits bindToStage, DataUsageFrequency usageFrequency, vk::DescriptorType type);

	vk::DescriptorSet GetDescriptor(DataUsageFrequency usageFrequency) const;
	vk::DescriptorSetLayout GetLayout(DataUsageFrequency usageFrequency) const;
	DescriptorInfo const* GetDescriptorInfo(DataUsageFrequency usageFrequency) const;
	vk::WriteDescriptorSet GetWriteDescriptor(DataUsageFrequency usageFrequency, uint32_t bindingId) const;

	std::vector<vk::DescriptorSet> GetDescriptors() const;

private:
	vk::raii::DescriptorPool m_descriptorPool;
	DescriptorSlotMap m_descriptorSlots;
	GfxDevicePtr_t m_pGfxDevice;
};

using GfxDescriptorManagerPtr_t = std::unique_ptr<GfxDescriptorManager>;

