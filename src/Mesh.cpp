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
#include <simple-animation-blender/Mesh.h>
using namespace Assimp;
using namespace glm;
using namespace std;
Mesh::Mesh(const char *path)
{
    //Importing mesh data...
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate);
    aiMesh *mesh = scene->mMeshes[0];
    vertices.resize(mesh->mNumVertices);
    for (uint32_t i = 0; i < vertices.size(); ++i)
    {
        vertices[i].pos = vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z);
        vertices[i].normal = vec3(
            mesh->mNormals[i].x,
            mesh->mNormals[i].y,
            mesh->mNormals[i].z);
    }
    bonesOffsets.resize(mesh->mNumBones);
    finalTransforms.resize(mesh->mNumBones);
    for (uint32_t i = 0; i < mesh->mNumBones; ++i)
    {
        bones.insert({mesh->mBones[i]->mName.data, i});
        bonesOffsets[i] = assimpToGlm(mesh->mBones[i]->mOffsetMatrix);
        for (uint32_t j = 0; j < mesh->mBones[i]->mNumWeights; ++j)
        {
            int vertexID = mesh->mBones[i]->mWeights[j].mVertexId;
            float weight = mesh->mBones[i]->mWeights[j].mWeight;
            if (weight != 0.0f)
            {
                if (vertices[vertexID].bonesIndices.x == -1)
                {
                    vertices[vertexID].bonesIndices.x = i;
                    vertices[vertexID].weights.x = weight;
                }
                else if (vertices[vertexID].bonesIndices.y == -1)
                {
                    vertices[vertexID].bonesIndices.y = i;
                    vertices[vertexID].weights.y = weight;
                }
                else if (vertices[vertexID].bonesIndices.z == -1)
                {
                    vertices[vertexID].bonesIndices.z = i;
                    vertices[vertexID].weights.z = weight;
                }
            }
        }
    }
    indices.resize(mesh->mNumFaces * 3);
    for (uint32_t i = 0; i < indices.size(); i += 3)
    {
        indices[i + 0] = mesh->mFaces[i / 3].mIndices[0];
        indices[i + 1] = mesh->mFaces[i / 3].mIndices[1];
        indices[i + 2] = mesh->mFaces[i / 3].mIndices[2];
    }
    animations.resize(scene->mNumAnimations);
    for (uint32_t i = 0; i < animations.size(); ++i)
    {
        animations[i].name = scene->mAnimations[i]->mName.data;
        animations[i].ticksPerSecond = scene->mAnimations[i]->mTicksPerSecond;
        animations[i].duration = scene->mAnimations[i]->mDuration;
        animations[i].channels.resize(scene->mAnimations[i]->mNumChannels);
        for (uint32_t j = 0; j < animations[i].channels.size(); ++j)
        {
            animations[i].channels[j].boneIndex = bones[scene->mAnimations[i]->mChannels[j]->mNodeName.data];
            animations[i].channels[j].posKeys.resize(scene->mAnimations[i]->mChannels[j]->mNumPositionKeys);
            animations[i].channels[j].rotKeys.resize(scene->mAnimations[i]->mChannels[j]->mNumRotationKeys);
            animations[i].channels[j].scaKeys.resize(scene->mAnimations[i]->mChannels[j]->mNumScalingKeys);
            for (uint32_t k = 0; k < animations[i].channels[j].posKeys.size(); ++k)
            {
                animations[i].channels[j].posKeys[k].pos = vec3{scene->mAnimations[i]->mChannels[j]->mPositionKeys[k].mValue.x,
                                                                scene->mAnimations[i]->mChannels[j]->mPositionKeys[k].mValue.y,
                                                                scene->mAnimations[i]->mChannels[j]->mPositionKeys[k].mValue.z};
                animations[i].channels[j].posKeys[k].time = scene->mAnimations[i]->mChannels[j]->mPositionKeys[k].mTime;
            }
            for (uint32_t k = 0; k < animations[i].channels[j].rotKeys.size(); ++k)
            {
                animations[i].channels[j].rotKeys[k].rot = quat{scene->mAnimations[i]->mChannels[j]->mRotationKeys[k].mValue.w,
                                                                scene->mAnimations[i]->mChannels[j]->mRotationKeys[k].mValue.x,
                                                                scene->mAnimations[i]->mChannels[j]->mRotationKeys[k].mValue.y,
                                                                scene->mAnimations[i]->mChannels[j]->mRotationKeys[k].mValue.z};
                animations[i].channels[j].rotKeys[k].time = scene->mAnimations[i]->mChannels[j]->mRotationKeys[k].mTime;
            }
            for (uint32_t k = 0; k < animations[i].channels[j].scaKeys.size(); ++k)
            {
                animations[i].channels[j].scaKeys[k].sca = vec3{scene->mAnimations[i]->mChannels[j]->mScalingKeys[k].mValue.x,
                                                                scene->mAnimations[i]->mChannels[j]->mScalingKeys[k].mValue.y,
                                                                scene->mAnimations[i]->mChannels[j]->mScalingKeys[k].mValue.z};
                animations[i].channels[j].scaKeys[k].time = scene->mAnimations[i]->mChannels[j]->mScalingKeys[k].mTime;
            }
        }
    }
    globalInverseTransform = assimpToGlm(scene->mRootNode->mTransformation);
    globalInverseTransform = inverse(globalInverseTransform);
    rootNode = scene->mRootNode;
    LOG("Imported vertices & indices & animation data successfully");
}
mat4 Mesh::assimpToGlm(aiMatrix4x4 matrix)
{
    return *((mat4 *)&matrix.Transpose());
}
Mesh::~Mesh()
{
    importer.FreeScene();
}