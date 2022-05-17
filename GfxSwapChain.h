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

	std::vector<vk::ImageView> GetImageViews() const {
		std::vector<vk::ImageView> views;
		views.reserve(m_imageViews.size());

		for (vk::raii::ImageView const& view : m_imageViews)
		{
			views.push_back(*view);
		}

		return views;
	}

	std::vector<vk::raii::ImageView> m_imageViews;
	vk::raii::SwapchainKHR m_swapchain;
	vk::Format m_format;
	vk::Extent2D m_extent;
};