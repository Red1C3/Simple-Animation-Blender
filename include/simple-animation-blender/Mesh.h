#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <simple-animation-blender/Logger.h>
#include <vulkan/vulkan.hpp>
class Mesh
{
private:
    struct ScaKey
    {
        double time;
        glm::vec3 sca;
    };
    struct RotKey
    {
        double time;
        glm::quat rot;
    };
    struct PosKey
    {
        double time;
        glm::vec3 pos;
    };
    struct Channel
    {
        int boneIndex;
        std::vector<PosKey> posKeys;
        std::vector<RotKey> rotKeys;
        std::vector<ScaKey> scaKeys;
    };
    struct Animation
    {
        std::vector<Channel> channels;
        double duration;
        std::string name;
    };
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::ivec3 bonesIndices = glm::ivec3(-1, -1, -1);
        glm::vec3 weights = glm::vec3(0, 0, 0);
    };
    struct UBO
    {
        glm::mat4 bones[50];
        glm::mat4 MVP;
        glm::vec3 color;
    };
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    std::vector<Animation> animations;
    VkBuffer vertexBuffer, indexBuffer, uniformBuffer;
    VkDeviceMemory vertexBufferMem, indexBufferMem, uniformBufferMem;
    VkDescriptorSet descriptorSet;
    UBO ubo;

public:
    Mesh(const char *path);
    friend class Application;
};