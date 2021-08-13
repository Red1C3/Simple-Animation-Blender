#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <vector>
#include <simple-animation-blender/Logger.h>
class Mesh
{
private:
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::ivec3 bonesIndices = glm::ivec3(-1, -1, -1);
        glm::vec3 weights = glm::vec3(0, 0, 0);
    };

public:
    std::vector<Vertex> vertcies;
    std::vector<uint16_t> indcies;
    Mesh(const char *path);
};