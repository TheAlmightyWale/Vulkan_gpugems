#pragma once

//dynamically load everything we need
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

//TODO move this out of the fwd declaration to speed up compile times
#include <vulkan/vulkan_raii.hpp>

using InstancePtr_t = std::shared_ptr<vk::raii::Instance>;
using ContextPtr_t = std::shared_ptr<vk::raii::Context>;
using DevicePtr_t = std::shared_ptr<vk::raii::Device>;

class GfxDevice;
using GfxDevicePtr_t = std::shared_ptr<GfxDevice>;

using SamplerPtr_t = std::shared_ptr<vk::raii::Sampler>;
struct GfxImage;
struct GfxUniformBuffer;
struct GfxPipeline;
struct GfxSwapchain;
class GfxEngine;
struct GfxFrame;
struct GfxBuffer;

struct VertexDescription
{
	std::vector<vk::VertexInputAttributeDescription> attributes;
	std::vector<vk::VertexInputBindingDescription> bindings;
};