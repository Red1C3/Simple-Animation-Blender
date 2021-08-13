#include <simple-animation-blender/Mesh.h>
using namespace Assimp;
using namespace glm;
Mesh::Mesh(const char *path)
{
    Importer importer;
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate);
    aiMesh *mesh = scene->mMeshes[0];
    vertcies.resize(mesh->mNumVertices);
    for (uint32_t i = 0; i < vertcies.size(); ++i)
    {
        vertcies[i].pos = vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z);
        vertcies[i].normal = vec3(
            mesh->mNormals[i].x,
            mesh->mNormals[i].y,
            mesh->mNormals[i].z);
    }
    for (uint32_t i = 0; i < mesh->mNumBones; ++i)
    {
        for (uint32_t j = 0; j < mesh->mBones[i]->mNumWeights; ++j)
        {
            int vertexID = mesh->mBones[i]->mWeights[j].mVertexId;
            float weight = mesh->mBones[i]->mWeights[j].mWeight;
            if (weight != 0.0f)
            {
                if (vertcies[vertexID].bonesIndices.x == -1)
                {
                    vertcies[vertexID].bonesIndices.x = i;
                    vertcies[vertexID].weights.x = weight;
                }
                else if (vertcies[vertexID].bonesIndices.y == -1)
                {
                    vertcies[vertexID].bonesIndices.y = i;
                    vertcies[vertexID].weights.y = weight;
                }
                else if (vertcies[vertexID].bonesIndices.z == -1)
                {
                    vertcies[vertexID].bonesIndices.z = i;
                    vertcies[vertexID].weights.z = weight;
                }
            }
        }
    }
    //TODO import indices
    //TODO import animations as well
    LOG("Imported vertcies data successfully");
    importer.FreeScene();
}