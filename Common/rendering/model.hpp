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
    Model(const Model& other) = delete;
    Model(Model&& other) = delete;
    Model& operator = (const Model& other) = delete;
    Model& operator = (Model&& other) = delete;

    void Draw(Shader& shader);
    ~Model();
private:
    // model data
    std::vector<Mesh> meshes;
    std::string directory;

    void loadModel(std::string path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<GLuint> loadMaterialTextures(aiMaterial* mat, aiTextureType type);
};

#endif