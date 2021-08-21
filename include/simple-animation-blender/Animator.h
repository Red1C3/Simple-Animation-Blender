#pragma once
#include <simple-animation-blender/Application.h>
#include <simple-animation-blender/Mesh.h>
class Animator
{
private:
    Animator();
    Mesh *mesh;
    Mesh::Animation playingAnimationOne, playingAnimationTwo;
    bool isPlaying = false;
    double animationTime;
    float blendingFactor = -1;
    void updateMeshNodes(const aiNode *node, const glm::mat4 &parentTransform);
    void blendMeshNodes(const aiNode *node, const glm::mat4 &parentTransform);
    int getKeyIndex(std::vector<Mesh::PosKey> keys, Mesh::Channel channel);
    int getKeyIndex(std::vector<Mesh::RotKey> keys, Mesh::Channel channel);
    int getKeyIndex(std::vector<Mesh::ScaKey> keys, Mesh::Channel channel);
    glm::vec3 interpolatePos(Mesh::Channel channel);
    aiQuaternion interpolateRot(Mesh::Channel channel);
    glm::vec3 interpolateSca(Mesh::Channel channel);

public:
    static Animator &instance();
    void init(Mesh *mesh);
    void play(std::string animation);
    void play(std::string firstAnim, std::string secondAnim, float blendingFactor);
    void animate(double timeSinceStart);
};