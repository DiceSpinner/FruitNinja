#ifndef MODEL_H
#define MODEL_H
#include "mesh.hpp"
#include "assimp/postprocess.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"

class Model
{
public:
    Model(const char* path);
    void Draw(Shader& shader);
private:
    // model data
    std::vector<Mesh> meshes;
    std::string directory;
    std::vector<Texture> loadedTextures;

    void loadModel(std::string path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type);
};

#endif