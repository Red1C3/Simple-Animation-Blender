#include <simple-animation-blender/Animator.h>
using namespace std;
using namespace glm;
Animator::Animator() {}
Animator &Animator::instance()
{
    static Animator animator;
    return animator;
}
void Animator::init(Mesh *mesh)
{
    this->mesh = mesh;
    mesh->ubo.MVP *= translate(mat4(1.0f), vec3(0, 20, 0)) * scale(mat4(1.0f), vec3(3, 3, 3)) * mat4(quat(vec3(0, 2.7f, 0))); //TODO remove
}
void Animator::play(string animation)
{
    for (uint32_t i = 0; i < mesh->animations.size(); ++i)
    {
        if (mesh->animations[i].name == animation)
        {
            playingAnimation = mesh->animations[i];
            isPlaying = true;
            break;
        }
    }
}
void Animator::animate(double timeSinceStart)
{
    if (!isPlaying)
        return;
    double ticksPerSecond = playingAnimation.ticksPerSecond != 0 ? playingAnimation.ticksPerSecond : 25.0;
    double timeInTicks = timeSinceStart * ticksPerSecond;
    animationTime = fmod(timeInTicks * 100.0, playingAnimation.duration);
    updateMeshNodes(mesh->rootNode, mat4(1.0f));
    for (uint32_t i = 0; i < mesh->bonesOffsets.size(); ++i)
    {
        mesh->ubo.bones[i] = mesh->finalTransforms[i];
    }
    Application::instance().updateUBO(mesh->ubo);
}
void Animator::updateMeshNodes(const aiNode *node, const glm::mat4 &parentTransform)
{
    string nodeName(node->mName.data);
    mat4 nodeTransform = Mesh::assimpToGlm(node->mTransformation);
    int channelIdx = -1;
    for (uint32_t i = 0; i < playingAnimation.channels.size(); ++i)
    {
        if (playingAnimation.channels[i].boneIndex == mesh->bones[nodeName])
        {
            channelIdx = i;
            break;
        }
    }
    if (channelIdx != -1)
    {
        mat4 traMat = interpolatePos(playingAnimation.channels[channelIdx]);
        mat4 rotMat = interpolateRot(playingAnimation.channels[channelIdx]);
        mat4 scaMat = interpolateSca(playingAnimation.channels[channelIdx]);
        nodeTransform = traMat * rotMat * scaMat;
    }
    mat4 globalTransform = parentTransform * nodeTransform;
    if (channelIdx != -1)
    {
        int boneIndex = mesh->bones[nodeName];
        mesh->finalTransforms[boneIndex] = mesh->globalInverseTransform * globalTransform * mesh->bonesOffsets[boneIndex];
    }
    for (uint32_t i = 0; i < node->mNumChildren; ++i)
    {
        updateMeshNodes(node->mChildren[i], globalTransform);
    }
}
mat4 Animator::interpolatePos(Mesh::Channel channel)
{
    if (channel.posKeys.size() == 1)
    {
        return translate(mat4(1.0f), channel.posKeys[0].pos);
    }
    int posIndex = getKeyIndex(channel.posKeys, channel);
    int nxtPosIndex = posIndex + 1;
    double deltaTime = channel.posKeys[nxtPosIndex].time - channel.posKeys[posIndex].time;
    float factor = (animationTime - channel.posKeys[posIndex].time) / deltaTime;
    vec3 interpolated = (1 - factor) * channel.posKeys[posIndex].pos +
                        factor * channel.posKeys[nxtPosIndex].pos;
    return translate(mat4(1.0f), interpolated);
}
mat4 Animator::interpolateRot(Mesh::Channel channel)
{
    if (channel.rotKeys.size() == 1)
    {
        return mat4(channel.rotKeys[0].rot);
    }
    int rotIndex = getKeyIndex(channel.rotKeys, channel);
    int nxtRotIndex = rotIndex + 1;
    double deltaTime = channel.rotKeys[nxtRotIndex].time - channel.rotKeys[rotIndex].time;
    float factor = (animationTime - channel.rotKeys[rotIndex].time) / deltaTime;
    quat startQuatGLM = channel.rotKeys[rotIndex].rot;
    aiQuaternion startQuatAssimp(startQuatGLM.w,
                                 startQuatGLM.x,
                                 startQuatGLM.y,
                                 startQuatGLM.z);
    quat endQuatGLM = channel.rotKeys[nxtRotIndex].rot;
    aiQuaternion endQuatAssimp(endQuatGLM.w,
                               endQuatGLM.x,
                               endQuatGLM.y,
                               endQuatGLM.z);
    aiQuaternion interpolatedAssimp;
    aiQuaternion::Interpolate(interpolatedAssimp, startQuatAssimp, endQuatAssimp, factor);
    interpolatedAssimp = interpolatedAssimp.Normalize();
    quat interpolatedGLM(interpolatedAssimp.w,
                         interpolatedAssimp.x,
                         interpolatedAssimp.y,
                         interpolatedAssimp.z);
    return mat4(interpolatedGLM);
}
mat4 Animator::interpolateSca(Mesh::Channel channel)
{
    if (channel.scaKeys.size() == 1)
    {
        return translate(mat4(1.0f), channel.scaKeys[0].sca);
    }
    int scaIndex = getKeyIndex(channel.scaKeys, channel);
    int nxtScaIndex = scaIndex + 1;
    double deltaTime = channel.scaKeys[nxtScaIndex].time - channel.scaKeys[scaIndex].time;
    float factor = (animationTime - channel.scaKeys[scaIndex].time) / deltaTime;
    vec3 interpolated = (1 - factor) * channel.scaKeys[scaIndex].sca +
                        factor * channel.scaKeys[nxtScaIndex].sca;
    return scale(mat4(1.0f), interpolated);
}
int Animator::getKeyIndex(vector<Mesh::PosKey> keys, Mesh::Channel channel)
{
    for (uint32_t i = 0; i < channel.posKeys.size(); ++i)
    {
        if (animationTime < channel.posKeys[i + 1].time)
        {
            return i;
        }
    }
    return -1;
}
int Animator::getKeyIndex(vector<Mesh::RotKey> keys, Mesh::Channel channel)
{
    for (uint32_t i = 0; i < channel.rotKeys.size(); ++i)
    {
        if (animationTime < channel.rotKeys[i + 1].time)
        {
            return i;
        }
    }
    return -1;
}
int Animator::getKeyIndex(vector<Mesh::ScaKey> keys, Mesh::Channel channel)
{
    for (uint32_t i = 0; i < channel.scaKeys.size(); ++i)
    {
        if (animationTime < channel.scaKeys[i + 1].time)
        {
            return i;
        }
    }
    return -1;
}