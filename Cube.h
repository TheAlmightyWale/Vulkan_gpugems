#pragma once
#include <cstdint>

uint32_t constexpr const k_cubeVertexValuesCount = 3 * 8;
uint32_t constexpr const k_cubeNormalsValuesCount = 3 * 8;
uint32_t constexpr const k_cubeIndicesCount = 6 * 6;

float const k_cubeVertices[k_cubeVertexValuesCount] =
{
    0.0f, 0.0f, 0.0f,
    1, 0.0f, 0.0f,
    1, 1, 0.0f,
    0.0f, 1, 0.0f,
    0.0f, 0.0f, 1,
    1, 0.0f, 1,
    1, 1, 1,
    0.0f, 1, 1
};

float texCoords[2 * 6] =
{
    0, 0,
    1, 0,
    1, 1,
    0, 1
};

float const k_cubeNormals[k_cubeNormalsValuesCount] =
{
    0, 0, 1,
    1, 0, 0,
    0, 0, -1,
    -1, 0, 0,
    0, 1, 0,
    0, -1, 0,
    //Trash normals below
    0, 0, 0,
    0, 0, 0
};

uint16_t const k_cubeIndices[k_cubeIndicesCount] =
{
    0, 1, 3, 3, 1, 2,
    1, 5, 2, 2, 5, 6,
    5, 4, 6, 6, 4, 7,
    4, 0, 7, 7, 0, 3,
    3, 2, 7, 7, 2, 6,
    4, 5, 0, 0, 5, 1
};