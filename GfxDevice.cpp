#include "GfxDevice.h"
#include "GfxImage.h"
#include "GfxPipeline.h"
#include "GfxSwapChain.h"
#include "GfxBuffer.h"
#include "Exceptions.h"
#include "Logger.h"
#include <bitset>


uint32_t FindMemoryType(vk::PhysicalDeviceMemoryProperties const& memoryProperties, uint32_t typeBits, vk::MemoryPropertyFlags requirementsMask)
{
	uint32_t typeIndex = uint32_t(~0);
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((typeBits & 1) && ((memoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask))
		{
			typeIndex = i;
			break;
		}
		typeBits >>= 1;
	}

	//TODO figure out fallbacks rather than erroring out? maybe okay for that to be the callers responsibility?
	if (typeIndex == uint32_t(~0)) {
		throw InvalidStateException("Could not find desired memory type");
	}

	return typeIndex;
}

//This isnt great, would be nicer to have a generic way to check what member variables are not default and only compare them
// This also assumes RHS is a default construced object with onyl a couple of fields set, rahter than considering both LHS and RHS
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
	vk::PhysicalDeviceFeatures const& desiredFeatures,
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
		desiredFeaturesBits.feats = desiredFeatures;
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

DevicePtr_t CreateLogicalDevice(vk::raii::PhysicalDevice const& physicalDevice, std::vector<char const*> enabledExtensions, std::vector<char const*> enabledLayers)
{
	uint32_t graphicsQueueIndex = GetGraphicsQueueFamilyIndex(physicalDevice.getQueueFamilyProperties());
	float queuePriority = 0.0f; //lowest priority for now
	vk::DeviceQueueCreateInfo deviceQueueCreateInfo({} /*flags*/, graphicsQueueIndex, 1 /*queue count*/, &queuePriority);

	auto features = physicalDevice.getFeatures();
	vk::DeviceCreateInfo deviceCreateInfo( {} /*flags*/, deviceQueueCreateInfo, enabledLayers, enabledExtensions, & features);

	SPDLOG_INFO("Created logical device with enabled Extensions: {} enabled Layers {}", enabledExtensions, enabledLayers);
	auto device = std::make_shared<vk::raii::Device>(physicalDevice, deviceCreateInfo);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(**device);

	return device;
}

GfxDevice::GfxDevice(
	vk::raii::Instance const& pInstance,
	vk::PhysicalDeviceFeatures desiredFeatures,
	vk::PhysicalDeviceProperties desiredProperties,
	std::vector<char const*> enabledExtensions,
	std::vector<char const*> enabledLayers)
	: m_physcialDevice(ChoosePhysicalDevice(pInstance, desiredFeatures, desiredProperties, enabledExtensions))
	, m_pDevice(CreateLogicalDevice(m_physcialDevice, enabledExtensions, enabledLayers))
	, m_graphcsQueueFamilyIndex(GetGraphicsQueueFamilyIndex(m_physcialDevice.getQueueFamilyProperties()))
{
}

GfxDevice::~GfxDevice()
{
}

vk::Queue GfxDevice::GetGraphicsQueue() { return *m_pDevice->getQueue(m_graphcsQueueFamilyIndex, 0); }

GfxSwapchain GfxDevice::CreateSwapChain(vk::raii::SurfaceKHR const& surface, uint32_t desiredSwapchainSize)
{
	//TODO handle separate graphics and present queues
	uint32_t graphicsQueueFamilyIndex = GetGraphicsQueueFamilyIndex(m_physcialDevice.getQueueFamilyProperties());
	if (!m_physcialDevice.getSurfaceSupportKHR(graphicsQueueFamilyIndex, *surface))
	{
		throw InitializationException("Failed to create swapChain, graphics queue family does not support presentation");
	}

	//TODO handle image format choosing
	std::vector<vk::SurfaceFormatKHR> formats = m_physcialDevice.getSurfaceFormatsKHR(*surface);
	vk::Format format = formats.at(0).format;

	vk::SurfaceCapabilitiesKHR surfaceCapabilities = m_physcialDevice.getSurfaceCapabilitiesKHR(*surface);

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
		*surface,
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
	
	for (auto image : swapChainImages)
	{
		imageViewCreateInfo.image = static_cast<vk::Image>(image);
		gfxSwapchain.m_imageViews.push_back({*m_pDevice.get(), imageViewCreateInfo});
	}

	return gfxSwapchain;
}

GfxImage GfxDevice::CreateDepthStencil(uint32_t width, uint32_t height, vk::Format depthFormat)
{
	vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eDepth;
	vk::ImageCreateInfo depthStencilCreateInfo{
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

	vk::raii::Image depthImage(*m_pDevice.get(), depthStencilCreateInfo);

	vk::PhysicalDeviceMemoryProperties memoryProperties = m_physcialDevice.getMemoryProperties();
	vk::MemoryRequirements memoryRequirements = depthImage.getMemoryRequirements();

	uint32_t typeIndex = FindMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	vk::MemoryAllocateInfo memoryAllocateInfo(memoryRequirements.size, typeIndex);
	vk::raii::DeviceMemory depthMemory(*m_pDevice.get(), memoryAllocateInfo);
	depthImage.bindMemory(*depthMemory, 0 /*memory offset*/);

	vk::ImageViewCreateInfo depthViewCreateInfo(
		{}/*flags*/,
		*depthImage,
		vk::ImageViewType::e2D,
		depthFormat,
		{} /*components*/,
		vk::ImageSubresourceRange{
			vk::ImageAspectFlagBits::eDepth,
			0 /*base mip level*/,
			1 /*level count*/,
			0 /*base array layer*/,
			1 /*layer count*/
		}
	);
	vk::raii::ImageView depthView(*m_pDevice.get(), depthViewCreateInfo);

	SPDLOG_INFO("Created depth buffer with dimensions x:{0}, y:{1}", width, height);

	GfxImage depthStencil;
	depthStencil.image = std::move(depthImage);
	depthStencil.view = std::move(depthView);
	depthStencil.memory = std::move(depthMemory);
	return depthStencil;
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

GfxBuffer GfxDevice::CreateBuffer(size_t size, vk::BufferUsageFlags flags)
{
	vk::BufferCreateInfo createInfo(
		{},
		size,
		flags,
		vk::SharingMode::eExclusive
	);

	vk::raii::Buffer buffer(*m_pDevice, createInfo);

	vk::DeviceBufferMemoryRequirements devBuffMemReq(&createInfo);
	vk::MemoryRequirements memoryRequirements = buffer.getMemoryRequirements();
	uint32_t memoryTypeIndex = FindMemoryType(m_physcialDevice.getMemoryProperties(), memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

	uint32_t finalSize = size >= memoryRequirements.size ? size : memoryRequirements.size;
	vk::MemoryAllocateInfo allocateInfo(finalSize, memoryTypeIndex);

	vk::raii::DeviceMemory memory = m_pDevice->allocateMemory(allocateInfo);
	vk::BindBufferMemoryInfo bindInfo(*buffer, *memory, 0 /*offset*/);
	m_pDevice->bindBufferMemory2(bindInfo);

	void* pData = memory.mapMemory(0 /*offset*/, size, {} /*flags*/);

	GfxBuffer result;
	result.m_buffer = std::move(buffer);
	result.m_memory = std::move(memory);
	result.m_dataSize = size;
	result.m_pData = pData;

	return std::move(result);
}

vk::raii::QueryPool GfxDevice::CreateQueryPool(uint32_t queryCount)
{
	vk::QueryPoolCreateInfo createInfo({}, vk::QueryType::eTimestamp, queryCount);
	vk::raii::QueryPool pool(*m_pDevice, createInfo);
	return std::move(pool);
}

vk::raii::CommandPool GfxDevice::CreateGraphicsCommandPool()
{
	uint32_t queueFamilyIndex = GetGraphicsQueueFamilyIndex(m_physcialDevice.getQueueFamilyProperties());
	vk::CommandPoolCreateInfo const createInfo({}/*flags*/, queueFamilyIndex);
	return std::move(vk::raii::CommandPool(*m_pDevice.get(), createInfo));
}

vk::raii::CommandBuffers GfxDevice::CreateCommandBuffers(vk::raii::CommandPool const& commandPool, uint32_t numBuffers)
{
	vk::CommandBufferAllocateInfo allocateInfo(*commandPool, vk::CommandBufferLevel::ePrimary, 1/*Command buffer count*/);
	return std::move(vk::raii::CommandBuffers(*m_pDevice.get(), allocateInfo));
}