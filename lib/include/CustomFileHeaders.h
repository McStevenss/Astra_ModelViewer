#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "glad/glad.h"
#include <cmath>
#include <string>
#include <fstream>
#include <iostream>


#pragma pack(push,1)
struct ModelHeader {
    char magic[4] = { 'M', 'O', 'D', 'L' }; // identifier
    uint32_t numMeshes;
    bool hasDiffuse;
    bool hasSpecular;
    bool hasNormal;
};
#pragma pack(pop)

#pragma pack(push,1)
struct MeshHeader {
    uint32_t numVertices;
    uint32_t numIndices;
    uint32_t numTextures;
    
    glm::vec3 aabbMin;
    glm::vec3 aabbMax;
};
#pragma pack(pop)

#pragma pack(push,1)
struct VertexBinary {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};
#pragma pack(pop)