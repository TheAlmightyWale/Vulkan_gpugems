#pragma once
#include "Mesh.h"
#include <string>

class ModelLoader {
public:
	static MeshPtr_t LoadCube(GfxDevicePtr_t const pDevice);
	static MeshPtr_t LoadModel(GfxDevicePtr_t const pDevice, std::string const& filePath);
};