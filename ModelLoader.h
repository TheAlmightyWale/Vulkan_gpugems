#pragma once
#include "Mesh.h"
#include <string>

class ModelLoader {
public:
	static Mesh LoadModel(std::string const& filePath);
};