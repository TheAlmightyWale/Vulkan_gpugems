#include "ImageLoader.h"

#include "Exceptions.h"
#include "GfxDevice.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

ImagePtr_t ImageLoader::LoadTexture(GfxDevice& device, std::string const& filePath)
{
    int width, height, channels;
    stbi_uc* const pixels = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels)
    {
        throw InvalidStateException("Failed to load image at: " + filePath);
    }

    size_t const imageSize = (size_t)width * (size_t)height * (size_t)channels;
    GfxBuffer buffer = device.CreateBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc);

    memcpy(buffer.m_pData, pixels, imageSize);
    stbi_image_free(pixels);

    ImagePtr_t pImage = std::make_shared<Image>();
    pImage->width = width;
    pImage->height = height;
    pImage->data = std::move(buffer);

    switch (channels)
    {
    case 3:
        pImage->format = vk::Format::eR8G8B8Srgb;
        break;
    case 4:
        pImage->format = vk::Format::eR8G8B8A8Srgb;
        break;
    default:
        throw InvalidStateException("Loading Image with unsupported number of color channels at: " + filePath);
        break;
    }

    return pImage;
}
