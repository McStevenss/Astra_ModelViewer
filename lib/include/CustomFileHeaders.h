#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "glad/glad.h"
#include <cmath>
#include <string>
#include <fstream>
#include <iostream>
// #include <limits>
// #include <cmath>


#pragma pack(push,1)
struct ModelHeader
{
    char magic[4];
    uint32_t size;
    float cell; 
    uint32_t gridX;
    uint32_t gridZ;
};
#pragma pack(pop)
