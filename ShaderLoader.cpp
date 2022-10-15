#include "ShaderLoader.h"

#include <fstream>

#include "Exceptions.h"
#include "Logger.h"
#include "GfxDevice.h"

vk::raii::ShaderModule ShaderLoader::LoadModule(std::string const& filePath, GfxDevicePtr_t pDevice)
{
	//TODO RAII files
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw InvalidStateException("Failed to load shader module at: " + filePath);
	}

	//Since cursor starts at end we can get file size by reading it's position
	size_t fileSize = (size_t)file.tellg();

	//spirv expects buffers to be measure in 32bits
	std::vector<uint32_t> buffer(fileSize / (sizeof(uint32_t)));

	file.seekg(0);
	file.read((char*)buffer.data(), fileSize);
	file.close();

	//size is expected to be in 32bit size
	vk::ShaderModuleCreateInfo createInfo({}, buffer.size() * sizeof(uint32_t), buffer.data());
	vk::raii::ShaderModule shaderModule(pDevice->GetDevice(), createInfo);

	SPDLOG_INFO("Loaded shader found at: " + filePath);

	return std::move(shaderModule);
}
