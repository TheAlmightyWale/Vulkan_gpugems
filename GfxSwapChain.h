#pragma once
#include "GfxFwdDecl.h"

struct GfxSwapchain
{
	GfxSwapchain()
		: m_imageViews()
		, m_swapchain(nullptr)
		, m_format(vk::Format::eUndefined)
		, m_extent(0,0)
	{}

	vk::ImageView GetImageView(uint32_t index) const {
		return *m_imageViews.at(index);
	}

	uint32_t Size() const { return m_imageViews.size(); }

	std::vector<vk::raii::ImageView> m_imageViews;
	vk::raii::SwapchainKHR m_swapchain;
	vk::Format m_format;
	vk::Extent2D m_extent;
};