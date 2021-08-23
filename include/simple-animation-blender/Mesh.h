/*MIT License

Copyright (c) 2021 Mohammad Issawi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/
#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <map>
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
        double duration, ticksPerSecond;
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
    //Inverse bind matrices for the bones
    std::vector<glm::mat4> bonesOffsets;
    std::map<std::string, int> bones;
    //Bones after applying an blended/animation
    std::vector<glm::mat4> finalTransforms;
    const aiNode *rootNode;
    Assimp::Importer importer;
    VkDeviceMemory vertexBufferMem, indexBufferMem, uniformBufferMem;

public:
    Mesh(const char *path);
    ~Mesh();
    UBO ubo;
    std::vector<Animation> animations;
    std::vector<uint16_t> indices;
    VkBuffer vertexBuffer, indexBuffer, uniformBuffer;
    VkDescriptorSet descriptorSet;
    //Casts an Assimp matrix into a GLM matrix
    static glm::mat4 assimpToGlm(aiMatrix4x4 matrix);
    friend class Application;
    friend class Animator;
};