#pragma once

#include "GfxFwdDecl.h"
#include "GfxBuffer.h"

struct Image
{
	GfxBuffer data;
	uint32_t height;
	uint32_t width;
	vk::Format format;
};

using ImagePtr_t = std::shared_ptr<Image>;

class ImageLoader
{
public:
	static ImagePtr_t LoadTexture(GfxDevice& device, std::string const& filePath);
};

