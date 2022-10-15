#pragma once
#include "GfxFwdDecl.h"

class ShaderLoader
{
public:
	static vk::raii::ShaderModule LoadModule(std::string const& filePath, GfxDevicePtr_t pDevice);
};