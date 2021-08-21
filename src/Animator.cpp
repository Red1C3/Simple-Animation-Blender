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
            playingAnimationOne = mesh->animations[i];
            isPlaying = true;
            blendingFactor = -1;
            break;
        }
    }
}
void Animator::play(string firstAnim, string secondAnim, float blendingFactor)
{
    for (uint32_t i = 0; i < mesh->animations.size(); ++i)
    {
        if (mesh->animations[i].name == firstAnim)
        {
            playingAnimationOne = mesh->animations[i];
        }
        if (mesh->animations[i].name == secondAnim)
        {
            playingAnimationTwo = mesh->animations[i];
        }
    }
    this->blendingFactor = blendingFactor;
    isPlaying = true;
}
void Animator::animate(double timeSinceStart)
{
    if (!isPlaying)
        return;
    double ticksPerSecond = playingAnimationOne.ticksPerSecond != 0 ? playingAnimationOne.ticksPerSecond : 25.0;
    double timeInTicks = timeSinceStart * ticksPerSecond;
    if (blendingFactor == -1)
    {
        animationTime = fmod(timeInTicks * 100.0, playingAnimationOne.duration);
        updateMeshNodes(mesh->rootNode, mat4(1.0f));
        for (uint32_t i = 0; i < mesh->bonesOffsets.size(); ++i)
        {
            mesh->ubo.bones[i] = mesh->finalTransforms[i];
        }
        Application::instance().updateUBO(mesh->ubo);
    }
    else
    {
        animationTime = fmod(timeInTicks * 100.0, std::max(playingAnimationOne.duration,
                                                           playingAnimationTwo.duration));
        blendMeshNodes(mesh->rootNode, mat4(1.0f));
        for (uint32_t i = 0; i < mesh->bonesOffsets.size(); ++i)
        {
            mesh->ubo.bones[i] = mesh->finalTransforms[i];
        }
        Application::instance().updateUBO(mesh->ubo);
    }
}
void Animator::blendMeshNodes(const aiNode *node, const glm::mat4 &parentTransform)
{
    string nodeName(node->mName.data);
    mat4 nodeTransform = Mesh::assimpToGlm(node->mTransformation);
    int channelIdxOne = -1, channelIdxTwo = -1;
    for (uint32_t i = 0; i < playingAnimationOne.channels.size(); ++i)
    {
        if (playingAnimationOne.channels[i].boneIndex == mesh->bones[nodeName])
        {
            channelIdxOne = i;
            break;
        }
    }
    for (uint32_t i = 0; i < playingAnimationTwo.channels.size(); ++i)
    {
        if (playingAnimationTwo.channels[i].boneIndex == mesh->bones[nodeName])
        {
            channelIdxTwo = i;
            break;
        }
    }
    if (channelIdxOne != -1 && channelIdxTwo != -1)
    {
        vec3 posOne = interpolatePos(playingAnimationOne.channels[channelIdxOne]);
        vec3 posTwo = interpolatePos(playingAnimationTwo.channels[channelIdxTwo]);
        aiQuaternion rotOne = interpolateRot(playingAnimationOne.channels[channelIdxOne]);
        aiQuaternion rotTwo = interpolateRot(playingAnimationTwo.channels[channelIdxTwo]);
        vec3 scaOne = interpolateSca(playingAnimationOne.channels[channelIdxOne]);
        vec3 scaTwo = interpolateSca(playingAnimationTwo.channels[channelIdxTwo]);
        vec3 pos = (1 - blendingFactor) * posOne + blendingFactor * posTwo;
        vec3 sca = (1 - blendingFactor) * scaOne + blendingFactor * scaTwo;
        aiQuaternion rot(1, 1, 1, 1);
        aiQuaternion::Interpolate(rot, rotOne, rotTwo, blendingFactor);
        mat4 traMat = translate(mat4(1.0f), pos);
        mat4 scaMat = scale(mat4(1.0f), sca);
        quat rotGLM(rot.w,
                    rot.x,
                    rot.y,
                    rot.z);
        mat4 rotMat = mat4(rotGLM);
        nodeTransform = traMat * rotMat * scaMat;
    }
    else if (channelIdxOne != -1)
    {
        mat4 traMat = translate(mat4(1.0f), interpolatePos(playingAnimationOne.channels[channelIdxOne]));
        mat4 scaMat = scale(mat4(1.0f), interpolateSca(playingAnimationOne.channels[channelIdxOne]));
        aiQuaternion rotAssimp = interpolateRot(playingAnimationOne.channels[channelIdxOne]);
        quat rotGLM(rotAssimp.w,
                    rotAssimp.x,
                    rotAssimp.y,
                    rotAssimp.z);
        mat4 rotMat = mat4(rotGLM);
        nodeTransform = traMat * rotMat * scaMat;
    }
    else if (channelIdxTwo != -1)
    {
        mat4 traMat = translate(mat4(1.0f), interpolatePos(playingAnimationTwo.channels[channelIdxTwo]));
        mat4 scaMat = scale(mat4(1.0f), interpolateSca(playingAnimationTwo.channels[channelIdxTwo]));
        aiQuaternion rotAssimp = interpolateRot(playingAnimationTwo.channels[channelIdxTwo]);
        quat rotGLM(rotAssimp.w,
                    rotAssimp.x,
                    rotAssimp.y,
                    rotAssimp.z);
        mat4 rotMat = mat4(rotGLM);
        nodeTransform = traMat * rotMat * scaMat;
    }
    mat4 globalTransform = parentTransform * nodeTransform;
    if (channelIdxOne != -1 || channelIdxTwo != -1)
    {
        int boneIndex = mesh->bones[nodeName];
        mesh->finalTransforms[boneIndex] = mesh->globalInverseTransform * globalTransform * mesh->bonesOffsets[boneIndex];
    }
    for (uint32_t i = 0; i < node->mNumChildren; ++i)
    {
        blendMeshNodes(node->mChildren[i], globalTransform);
    }
}
void Animator::updateMeshNodes(const aiNode *node, const glm::mat4 &parentTransform)
{
    string nodeName(node->mName.data);
    mat4 nodeTransform = Mesh::assimpToGlm(node->mTransformation);
    int channelIdx = -1;
    for (uint32_t i = 0; i < playingAnimationOne.channels.size(); ++i)
    {
        if (playingAnimationOne.channels[i].boneIndex == mesh->bones[nodeName])
        {
            channelIdx = i;
            break;
        }
    }
    if (channelIdx != -1)
    {
        mat4 traMat = translate(mat4(1.0f), interpolatePos(playingAnimationOne.channels[channelIdx]));
        mat4 scaMat = scale(mat4(1.0f), interpolateSca(playingAnimationOne.channels[channelIdx]));
        aiQuaternion rotAssimp = interpolateRot(playingAnimationOne.channels[channelIdx]);
        quat rotGLM(rotAssimp.w,
                    rotAssimp.x,
                    rotAssimp.y,
                    rotAssimp.z);
        mat4 rotMat = mat4(rotGLM);
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
vec3 Animator::interpolatePos(Mesh::Channel channel)
{
    if (channel.posKeys.size() == 1)
    {
        return channel.posKeys[0].pos;
    }
    int posIndex = getKeyIndex(channel.posKeys, channel);
    int nxtPosIndex = posIndex + 1;
    double deltaTime = channel.posKeys[nxtPosIndex].time - channel.posKeys[posIndex].time;
    float factor = (animationTime - channel.posKeys[posIndex].time) / deltaTime;
    vec3 interpolated = (1 - factor) * channel.posKeys[posIndex].pos +
                        factor * channel.posKeys[nxtPosIndex].pos;
    return interpolated;
}
aiQuaternion Animator::interpolateRot(Mesh::Channel channel)
{
    if (channel.rotKeys.size() == 1)
    {
        return aiQuaternion(channel.rotKeys[0].rot.w,
                            channel.rotKeys[0].rot.x,
                            channel.rotKeys[0].rot.y,
                            channel.rotKeys[0].rot.z);
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
    return interpolatedAssimp;
}
vec3 Animator::interpolateSca(Mesh::Channel channel)
{
    if (channel.scaKeys.size() == 1)
    {
        return channel.scaKeys[0].sca;
    }
    int scaIndex = getKeyIndex(channel.scaKeys, channel);
    int nxtScaIndex = scaIndex + 1;
    double deltaTime = channel.scaKeys[nxtScaIndex].time - channel.scaKeys[scaIndex].time;
    float factor = (animationTime - channel.scaKeys[scaIndex].time) / deltaTime;
    vec3 interpolated = (1 - factor) * channel.scaKeys[scaIndex].sca +
                        factor * channel.scaKeys[nxtScaIndex].sca;
    return interpolated;
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
    return channel.posKeys.size() - 2;
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
    return channel.rotKeys.size() - 2;
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
    return channel.scaKeys.size() - 2;
}