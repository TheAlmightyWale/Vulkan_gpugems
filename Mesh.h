#pragma once
#include <vector>

struct Vertex
{
	float vx, vy, vz;
	float nx, ny, nz;
};

struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
};