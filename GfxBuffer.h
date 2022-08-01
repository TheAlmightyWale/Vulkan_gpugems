#pragma once
#include "GfxFwdDecl.h"

struct GfxBuffer
{
	GfxBuffer();

	vk::raii::Buffer m_buffer;
	vk::raii::DeviceMemory m_memory;
	void* m_pData;
	size_t m_dataSize;
};