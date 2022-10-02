#include "GfxBuffer.h"

GfxBuffer::GfxBuffer()
	: m_buffer(nullptr)
	, m_memory(nullptr)
	, m_pData(nullptr)
	, m_dataSize(0)
{
}

size_t GfxBuffer::CopyToBuffer(void const*  pSrc, size_t bytesToCopy, size_t writeOffset)
{
	uint8_t* pDestination = static_cast<uint8_t*>(m_pData) + writeOffset;
	memcpy(pDestination, pSrc, bytesToCopy);
	return writeOffset + bytesToCopy;
}

