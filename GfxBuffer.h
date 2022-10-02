#pragma once
#include "GfxFwdDecl.h"

//TODO Buffers can handle their own memory management and don't need to be totally public structs
struct GfxBuffer
{
	GfxBuffer();

	vk::raii::Buffer m_buffer;
	vk::raii::DeviceMemory m_memory;
	void* m_pData;
	size_t m_dataSize;

	//Returns new write offset, assumes caller will check if write offset + bytesToCopy is larger than datasize of buffer
	//Assumes pSrc is always a valid pointer
	size_t CopyToBuffer(void const* pSrc, size_t bytesToCopy, size_t writeOffset);
};