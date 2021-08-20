#pragma once
#include <simple-animation-blender/Application.h>
#include <simple-animation-blender/Mesh.h>
class Animator
{
private:
    Animator();
    Mesh *mesh;
    Mesh::Animation playingAnimation;
    bool isPlaying = false;
    double animationTime;
    void updateMeshNodes(const aiNode *node, const glm::mat4 &parentTransform);
    int getKeyIndex(std::vector<Mesh::PosKey> keys, Mesh::Channel channel);
    int getKeyIndex(std::vector<Mesh::RotKey> keys, Mesh::Channel channel);
    int getKeyIndex(std::vector<Mesh::ScaKey> keys, Mesh::Channel channel);
    glm::mat4 interpolatePos(Mesh::Channel channel);
    glm::mat4 interpolateRot(Mesh::Channel channel);
    glm::mat4 interpolateSca(Mesh::Channel channel);

public:
    static Animator &instance();
    void init(Mesh *mesh);
    void play(std::string animation);
    void animate(double timeSinceStart);
};