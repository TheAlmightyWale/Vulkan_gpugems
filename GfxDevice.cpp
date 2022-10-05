#include "GfxDevice.h"
#include "GfxImage.h"
#include "GfxPipeline.h"
#include "GfxSwapChain.h"
#include "GfxBuffer.h"
#include "Exceptions.h"
#include "Logger.h"
#include <bitset>


template<typename T>
constexpr auto SizeInBits(T a) { return sizeof(a) * 8; }

size_t GetAlignedSize(size_t desiredSize, vk::BufferUsageFlagBits bufferType, vk::PhysicalDeviceProperties const& deviceProperties)
{
	size_t largestUsageAlignment = 0;

	//Find the largest memory alignment that we need for our usage
	//Go through all bits that are set and get the relevant minimum memory alignment
	uint32_t bitIter = 1;
	for (uint32_t i = 0; i < SizeInBits(bufferType); i++)
	{
		//TODO must be a better way to do this iteration and value look up, especially if we only actually care about 2 of them?
		if ((bitIter & (uint32_t)bufferType) == bitIter)
		{
			switch (bitIter)
			{
			case (uint32_t)vk::BufferUsageFlagBits::eUniformBuffer:
				largestUsageAlignment = std::max(deviceProperties.limits.minUniformBufferOffsetAlignment,largestUsageAlignment);
				break;
			case (uint32_t)vk::BufferUsageFlagBits::eStorageBuffer:
				largestUsageAlignment = std::max(deviceProperties.limits.minStorageBufferOffsetAlignment, largestUsageAlignment);
				break;
			}
		}
		bitIter <<= 1;
	}

	//Calculate next largest alignment and return it
	if (largestUsageAlignment > 0)
	{
		return (desiredSize + largestUsageAlignment - 1) & ~(largestUsageAlignment - 1);
	}

	return desiredSize;
}

uint32_t FindMemoryType(vk::PhysicalDeviceMemoryProperties const& memoryProperties, uint32_t requiredTypeBits, vk::MemoryPropertyFlags requiredProperties)
{
	uint32_t typeIndex = uint32_t(~0);
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((requiredTypeBits & 1) && ((memoryProperties.memoryTypes.at(i).propertyFlags & requiredProperties) == requiredProperties))
		{
			typeIndex = i;
			break;
		}
		requiredTypeBits >>= 1;
	}

	//TODO figure out fallbacks rather than erroring out? maybe okay for that to be the callers responsibility?
	if (typeIndex == uint32_t(~0)) {
		throw InvalidStateException("Could not find desired memory type");
	}

	return typeIndex;
}

//TODO This isnt great, would be nicer to have a generic way to check what member variables are not default and only compare them
// This also assumes RHS is a default construced object with only a couple of fields set, rahter than considering both LHS and RHS
bool CompareOnlyNonDefaultFields(vk::PhysicalDeviceProperties const& actual, vk::PhysicalDeviceProperties const& desired)
{
	vk::PhysicalDeviceProperties defaultProps;
	bool bSameType = desired.deviceType == defaultProps.deviceType ? true : desired.deviceType == actual.deviceType;

	return (desired.apiVersion == defaultProps.apiVersion ? true : desired.apiVersion <= actual.apiVersion)
		&& (desired.deviceID == defaultProps.deviceID ? true : desired.deviceID == actual.deviceID)
		&& (desired.deviceName == defaultProps.deviceName ? true : desired.deviceName == actual.deviceName)
		&& (desired.deviceType == defaultProps.deviceType ? true : desired.deviceType == actual.deviceType)
		&& (desired.driverVersion == defaultProps.driverVersion ? true : desired.driverVersion == actual.driverVersion)
		&& (desired.limits == defaultProps.limits ? true : desired.limits == actual.limits)
		&& (desired.pipelineCacheUUID == defaultProps.pipelineCacheUUID ? true : desired.pipelineCacheUUID == actual.pipelineCacheUUID)
		&& (desired.sparseProperties == defaultProps.sparseProperties ? true : desired.sparseProperties == actual.sparseProperties)
		&& (desired.vendorID == defaultProps.vendorID ? true : desired.vendorID == actual.vendorID)
		&& (bSameType);
}

union PhysicalDeviceFeaturesBitSet
{
	vk::PhysicalDeviceFeatures feats;
	std::bitset<sizeof(vk::PhysicalDeviceFeatures)> bits;
};

vk::raii::PhysicalDevice ChoosePhysicalDevice(
	vk::raii::Instance const& pInstance,
	vk::PhysicalDeviceFeatures2 const& desiredFeatures,
	vk::PhysicalDeviceProperties const& desiredProperties,
	std::vector<char const*>const& desiredExtensionNames) {

	auto physicalDevices = vk::raii::PhysicalDevices(pInstance);

	//Enumerate physical devices to find best suited to needs demanded
	for (uint32_t index = 0; index < physicalDevices.size(); index++) {
		auto& physicalDevice = physicalDevices.at(index);
		vk::PhysicalDeviceProperties const properties = physicalDevice.getProperties();
		vk::PhysicalDeviceFeatures const features = physicalDevice.getFeatures();

		//TODO log physical device features and properties
		bool bHasAllProperties = CompareOnlyNonDefaultFields(properties, desiredProperties);

		PhysicalDeviceFeaturesBitSet featuresBits{};
		featuresBits.feats = features;
		PhysicalDeviceFeaturesBitSet desiredFeaturesBits{};
		desiredFeaturesBits.feats = desiredFeatures.features;
		PhysicalDeviceFeaturesBitSet comparedFeatures{};
		comparedFeatures.bits = featuresBits.bits & desiredFeaturesBits.bits;
		bool bHasAllFeatures = comparedFeatures.feats == desiredFeaturesBits.feats;

		//Check extensions 
		uint32_t foundExtensionCount = 0;
		for (auto const& extension : physicalDevice.enumerateDeviceExtensionProperties())
		{
			for (char const* extensionName : desiredExtensionNames)
			{
				if (std::strcmp(extensionName, extension.extensionName) == 0)
				{
					foundExtensionCount++;
					break;
				}
			}
		}
		bool bHasAllExtensions = foundExtensionCount == desiredExtensionNames.size();

		if (bHasAllProperties && bHasAllFeatures && bHasAllExtensions)
		{
			SPDLOG_INFO("Found Suitable Physical Device: {0}", properties.deviceName );

			return std::move(physicalDevice);
		}
	}

	// if we haven't picked a device throw an error
	throw InitializationException("Failed to find device with the desired features and properties");
}

uint32_t GetGraphicsQueueFamilyIndex(std::vector<vk::QueueFamilyProperties> const& queueFamilyProperties)
{
	auto graphicsQueueFamilyProperty = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
		[](vk::QueueFamilyProperties const& qfp) { return qfp.queueFlags & vk::QueueFlagBits::eGraphics; });

	if (graphicsQueueFamilyProperty == queueFamilyProperties.end())
	{
		throw new InitializationException("Could not find Queue Family that supports graphics");
	}

	return static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));
}

DevicePtr_t CreateLogicalDevice(vk::raii::PhysicalDevice const& physicalDevice, std::vector<char const*> enabledExtensions, std::vector<char const*> enabledLayers, vk::PhysicalDeviceFeatures2 features)
{
	uint32_t graphicsQueueIndex = GetGraphicsQueueFamilyIndex(physicalDevice.getQueueFamilyProperties());
	float queuePriority = 0.0f; //lowest priority for now
	vk::DeviceQueueCreateInfo deviceQueueCreateInfo({} /*flags*/, graphicsQueueIndex, 1 /*queue count*/, &queuePriority);

	//TODO just enabling everything right now. There are probably features we can disable which will give us better performance
	features.setFeatures(physicalDevice.getFeatures());
	vk::DeviceCreateInfo deviceCreateInfo( {} /*flags*/, deviceQueueCreateInfo, enabledLayers, enabledExtensions, nullptr, &features);

	SPDLOG_INFO("Created logical device with enabled Extensions: {} enabled Layers {}", enabledExtensions, enabledLayers);
	auto device = std::make_shared<vk::raii::Device>(physicalDevice, deviceCreateInfo);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(**device);

	return device;
}

GfxDevice::GfxDevice(
	vk::raii::Instance const& pInstance,
	vk::PhysicalDeviceFeatures2 desiredFeatures,
	vk::PhysicalDeviceProperties desiredProperties,
	std::vector<char const*> enabledExtensions,
	std::vector<char const*> enabledLayers)
	: m_physcialDevice(ChoosePhysicalDevice(pInstance, desiredFeatures, desiredProperties, enabledExtensions))
	, m_pDevice(CreateLogicalDevice(m_physcialDevice, enabledExtensions, enabledLayers, desiredFeatures))
	, m_graphcsQueueFamilyIndex(GetGraphicsQueueFamilyIndex(m_physcialDevice.getQueueFamilyProperties()))
{
}

GfxDevice::~GfxDevice()
{
}

vk::Queue GfxDevice::GetGraphicsQueue() { return *m_pDevice->getQueue(m_graphcsQueueFamilyIndex, 0); }

GfxSwapchain GfxDevice::CreateSwapChain(vk::SurfaceKHR const& surface, uint32_t desiredSwapchainSize)
{
	//TODO handle separate graphics and present queues
	uint32_t graphicsQueueFamilyIndex = GetGraphicsQueueFamilyIndex(m_physcialDevice.getQueueFamilyProperties());
	if (!m_physcialDevice.getSurfaceSupportKHR(graphicsQueueFamilyIndex, surface))
	{
		throw InitializationException("Failed to create swapChain, graphics queue family does not support presentation");
	}

	//TODO handle image format choosing
	std::vector<vk::SurfaceFormatKHR> formats = m_physcialDevice.getSurfaceFormatsKHR(surface);
	vk::Format format = formats.at(0).format;

	vk::SurfaceCapabilitiesKHR surfaceCapabilities = m_physcialDevice.getSurfaceCapabilitiesKHR(surface);

	//TODO handle surface not providing extents and us providing a default to fall back to
	if (surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max() || surfaceCapabilities.currentExtent.height == std::numeric_limits<uint32_t>::max())
	{
		throw InitializationException("Failed to create swapChain, surface did not provide valid extents");
	}

	if (surfaceCapabilities.minImageCount > desiredSwapchainSize || surfaceCapabilities.maxImageCount < desiredSwapchainSize)
	{
		throw InitializationException(
			std::format("Surface capabilities do not match desired swapchin size. min {0} max {1} desired {2}",
				surfaceCapabilities.minImageCount,
				surfaceCapabilities.maxImageCount,
				desiredSwapchainSize));
	}

	vk::Extent2D swapChainExtent = surfaceCapabilities.currentExtent;
	vk::PresentModeKHR swapChainPresentMode = vk::PresentModeKHR::eFifo;

	vk::SurfaceTransformFlagBitsKHR preTransform = (surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
		? vk::SurfaceTransformFlagBitsKHR::eIdentity
		: surfaceCapabilities.currentTransform;

	//TODO poll for supported transparency
	vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

	vk::SwapchainCreateInfoKHR createInfo(
		{}/*flags*/,
		surface,
		desiredSwapchainSize,
		format,
		vk::ColorSpaceKHR::eSrgbNonlinear, //TODO attributes of different color spaces?
		swapChainExtent,
		1, /*image array layers*/
		vk::ImageUsageFlagBits::eColorAttachment,
		vk::SharingMode::eExclusive,
		{}, /*queue family indices*/
		preTransform,
		compositeAlpha,
		swapChainPresentMode,
		true, /*clipped*/
		nullptr); /*oldSwapchain*/ //TODO delete old swap chain and associated image views on re-create

	vk::raii::SwapchainKHR swapChain(*m_pDevice.get(), createInfo, nullptr /*allocator*/);

	std::vector<VkImage> swapChainImages = swapChain.getImages();
	
	GfxSwapchain gfxSwapchain;
	gfxSwapchain.m_format = format;
	gfxSwapchain.m_swapchain = std::move(swapChain);
	gfxSwapchain.m_extent = swapChainExtent;
	gfxSwapchain.m_imageViews.reserve(swapChainImages.size());

	vk::ImageViewCreateInfo imageViewCreateInfo(
		{}/*flags*/,
		{}/*image*/,
		vk::ImageViewType::e2D,
		format,
		{} /*component mapping*/,
		vk::ImageSubresourceRange{ 
			vk::ImageAspectFlagBits::eColor /*aspect mask*/,
			0 /*base mip level*/,
			1 /*level count*/,
			0/*base array layer*/,
			1 /*layer count*/ }
	);
	
	for (VkImage const& image : swapChainImages)
	{
		imageViewCreateInfo.image = static_cast<vk::Image>(image);
		gfxSwapchain.m_imageViews.push_back({*m_pDevice.get(), imageViewCreateInfo});

		//TODO split out transfer / transition work into bulk worker queue
		//transition image to presentKHR
		vk::raii::CommandPool commandPool = CreateGraphicsCommandPool();
		vk::raii::CommandBuffer transitionBuffer = std::move(CreateCommandBuffers(*commandPool, 1).front());
		vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		transitionBuffer.begin(beginInfo);
		vk::ImageMemoryBarrier transitionBarrier = CreateImageTransition(
			vk::AccessFlagBits::eColorAttachmentWrite,
			vk::AccessFlagBits::eNone,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR,
			static_cast<vk::Image>(image)
		);
		transitionBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::DependencyFlagBits::eByRegion,
			nullptr, nullptr,
			transitionBarrier
		);
		transitionBuffer.end();

		vk::SubmitInfo submitInfo(nullptr, nullptr, *transitionBuffer, nullptr);
		GetGraphicsQueue().submit(submitInfo);
		m_pDevice->waitIdle();
	}

	return gfxSwapchain;
}

GfxImage GfxDevice::CreateDepthStencil(uint32_t width, uint32_t height, vk::Format depthFormat)
{
	vk::ImageAspectFlags const aspect = vk::ImageAspectFlagBits::eDepth;
	vk::ImageCreateInfo const depthStencilCreateInfo{
		{} /*flags*/,
		vk::ImageType::e2D,
		depthFormat,
		vk::Extent3D{width, height, 1/*depth*/},
		1 /*mip level*/,
		1 /*array layers*/,
		vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal, //TODO check image tiling based on depth format properties 
		vk::ImageUsageFlagBits::eDepthStencilAttachment
	};

	SPDLOG_DEBUG("Creating Depth Buffer");

	return CreateImage(depthStencilCreateInfo, aspect, vk::MemoryPropertyFlagBits::eDeviceLocal);
}

GfxImage GfxDevice::CreateImage(vk::ImageCreateInfo createInfo, vk::ImageAspectFlags aspect, vk::MemoryPropertyFlagBits desiredMemoryProperties)
{
	GfxImage image;
	image.image = vk::raii::Image(*m_pDevice.get(), createInfo);

	vk::PhysicalDeviceMemoryProperties memoryProperties = m_physcialDevice.getMemoryProperties();
	vk::MemoryRequirements memoryRequirements = image.image.getMemoryRequirements();

	uint32_t typeIndex = FindMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, desiredMemoryProperties);

	vk::MemoryAllocateInfo memoryAllocateInfo(memoryRequirements.size, typeIndex);
	image.memory = vk::raii::DeviceMemory(*m_pDevice.get(), memoryAllocateInfo);
	image.image.bindMemory(*image.memory, 0 /*memory offset*/);

	vk::ImageViewCreateInfo imageViewCreateInfo(
		{}/*flags*/,
		* image.image,
		vk::ImageViewType::e2D,
		createInfo.format,
		{} /*components*/,
		vk::ImageSubresourceRange{
			aspect,
			0 /*base mip level*/,
			1 /*level count*/,
			0 /*base array layer*/,
			1 /*layer count*/
		}
	);
	image.view = vk::raii::ImageView(*m_pDevice.get(), imageViewCreateInfo);
	image.extent = createInfo.extent;

	SPDLOG_DEBUG("Created image resource with dimensions x:{0}, y:{1}", createInfo.extent.width, createInfo.extent.height);

	return image;
}

vk::ImageMemoryBarrier GfxDevice::CreateImageTransition(vk::AccessFlagBits sourceAccess, vk::AccessFlagBits destinationAccess, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::Image image)
{
	vk::ImageSubresourceRange range(vk::ImageAspectFlagBits::eColor, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS);
	vk::ImageMemoryBarrier barrier(
		sourceAccess,
		destinationAccess,
		oldLayout,
		newLayout,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		image,
		range
	);
	return barrier;
}

vk::raii::Semaphore GfxDevice::CreateVkSemaphore()
{
	vk::SemaphoreCreateInfo createInfo({});
	vk::raii::Semaphore semaphore(*m_pDevice.get(), createInfo);

	return std::move(semaphore);
}

vk::raii::Fence GfxDevice::CreateFence()
{
	vk::FenceCreateInfo createInfo(vk::FenceCreateFlagBits::eSignaled);
	vk::raii::Fence fence(*m_pDevice, createInfo);
	return std::move(fence);
}

//CreateBuffer takes in the size of the data, but the actual buffer allocation may be larger due to alignment
GfxBuffer GfxDevice::CreateBuffer(size_t dataSize, vk::BufferUsageFlagBits flags) noexcept
{
	size_t alignedSize = GetAlignedSize(dataSize, flags, GetProperties());

	vk::BufferCreateInfo createInfo(
		{},
		alignedSize,
		flags,
		vk::SharingMode::eExclusive
	);

	vk::raii::Buffer buffer(*m_pDevice, createInfo);

	vk::DeviceBufferMemoryRequirements devBuffMemReq(&createInfo);
	vk::MemoryRequirements memoryRequirements = buffer.getMemoryRequirements();
	uint32_t memoryTypeIndex = FindMemoryType(m_physcialDevice.getMemoryProperties(), memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	uint32_t finalSize = alignedSize >= memoryRequirements.size ? alignedSize : memoryRequirements.size;
	vk::MemoryAllocateInfo allocateInfo(finalSize, memoryTypeIndex);

	vk::raii::DeviceMemory memory = m_pDevice->allocateMemory(allocateInfo);
	vk::BindBufferMemoryInfo bindInfo(*buffer, *memory, 0 /*offset*/);
	m_pDevice->bindBufferMemory2(bindInfo);

	void* pData = memory.mapMemory(0 /*offset*/, alignedSize, {} /*flags*/);

	GfxBuffer result;
	result.m_buffer = std::move(buffer);
	result.m_memory = std::move(memory);
	result.m_dataSize = alignedSize;
	result.m_pData = pData;

	return std::move(result);
}

vk::raii::QueryPool GfxDevice::CreateQueryPool(uint32_t queryCount)
{
	vk::QueryPoolCreateInfo createInfo({}, vk::QueryType::eTimestamp, queryCount);
	vk::raii::QueryPool pool(*m_pDevice, createInfo);
	return std::move(pool);
}

vk::raii::Sampler GfxDevice::CreateTextureSampler()
{
	vk::SamplerCreateInfo createInfo(
		{},
		vk::Filter::eLinear /*mag filter*/,
		vk::Filter::eLinear /*min filter*/,
		vk::SamplerMipmapMode::eLinear /*mipmap mode*/,
		/* U,V,W respectively*/
		vk::SamplerAddressMode::eRepeat,
		vk::SamplerAddressMode::eRepeat,
		vk::SamplerAddressMode::eRepeat,
		0.0f /*mip LOD bias*/,
		VK_FALSE /*anisotropy enable*/,
		0.0f /* max anisotropy*/,
		VK_FALSE /* compare enable*/,
		vk::CompareOp::eNever,
		0.0f /*min LOD*/,
		1.0f /*max LOD*/,
		vk::BorderColor::eFloatOpaqueWhite
	);
	return std::move(vk::raii::Sampler(*m_pDevice, createInfo));
}

vk::raii::CommandPool GfxDevice::CreateGraphicsCommandPool()
{
	uint32_t queueFamilyIndex = GetGraphicsQueueFamilyIndex(m_physcialDevice.getQueueFamilyProperties());
	vk::CommandPoolCreateInfo const createInfo({}/*flags*/, queueFamilyIndex);
	return std::move(vk::raii::CommandPool(*m_pDevice.get(), createInfo));
}

vk::raii::CommandBuffers GfxDevice::CreateCommandBuffers(vk::CommandPool commandPool, uint32_t numBuffers)
{
	vk::CommandBufferAllocateInfo allocateInfo(commandPool, vk::CommandBufferLevel::ePrimary, numBuffers);
	return std::move(vk::raii::CommandBuffers(*m_pDevice.get(), allocateInfo));
}

void GfxDevice::UploadBufferData(size_t bytesToUpload, size_t bufferOffset, vk::Buffer copyFromBuffer, vk::DescriptorSet descriptorSet, uint32_t bindingId, vk::DescriptorType type)
{
	vk::DescriptorBufferInfo copyBufferInfo(copyFromBuffer, bufferOffset, bytesToUpload);
	vk::WriteDescriptorSet writeDescriptor(
		descriptorSet,
		bindingId,
		0,
		type,
		nullptr, copyBufferInfo, nullptr,
		nullptr
	);
	m_pDevice->updateDescriptorSets(writeDescriptor, nullptr);
}

void GfxDevice::UploadImageData(vk::CommandPool commandPool, vk::Queue submitQueue, GfxImage const& image, GfxBuffer const& imageData)
{
	vk::raii::CommandBuffer copyCommands = std::move(CreateCommandBuffers(commandPool, 1).front());
	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	copyCommands.begin(beginInfo);

	vk::ImageMemoryBarrier preCopyBarrier = CreateImageTransition( 
		vk::AccessFlagBits::eNone,
		vk::AccessFlagBits::eTransferWrite,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eTransferDstOptimal,
		*image.image
	);

	copyCommands.pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eTransfer,
		vk::DependencyFlagBits::eByRegion,
		nullptr,
		nullptr,
		preCopyBarrier
	);

	//Do actual upload
	vk::BufferImageCopy copyRegion(
		0 /*offset*/,
		0 /*buffer row length*/,
		0 /*buffer image height*/,
		vk::ImageSubresourceLayers(
			vk::ImageAspectFlagBits::eColor,
			0/* mip level*/,
			0/* base array layer*/,
			1/* layer count*/
		),
		vk::Offset3D(0, 0, 0),
		image.extent
	);

	copyCommands.copyBufferToImage(
		*imageData.m_buffer,
		*image.image,
		vk::ImageLayout::eTransferDstOptimal,
		copyRegion
	);

	vk::ImageMemoryBarrier postCopyMemoryBarrier = CreateImageTransition(
		vk::AccessFlagBits::eTransferWrite,
		vk::AccessFlagBits::eShaderRead,
		//Transfer layout to shader read only
		vk::ImageLayout::eTransferDstOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal,
		*image.image
	);

	copyCommands.pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eFragmentShader,
		vk::DependencyFlagBits::eByRegion,
		nullptr,
		nullptr,
		postCopyMemoryBarrier
	);

	copyCommands.end();

	vk::SubmitInfo submitInfo(nullptr, nullptr, *copyCommands, nullptr);
	submitQueue.submit(submitInfo);

	//TODO better synchronization rather than waiting idle
	submitQueue.waitIdle();
}
