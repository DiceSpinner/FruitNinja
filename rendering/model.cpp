#include <set>
#include <iostream>
#include "model.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext.hpp"

using namespace std;

static glm::vec3 toVec3(const aiVector3D vector) {
	return glm::vec3(vector.x, vector.y, vector.z);
}

Model::Model(const char* path) {
	{
		loadModel(path);
	}
}

void Model::Draw(Shader& shader){
	for (auto& mesh : meshes) {
		mesh.Draw(shader);
	}
}

void Model::loadModel(string path) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		cout << "ERROR::ASSIMP::" << importer.GetErrorString() << endl;
		return;
	}
	directory = path.substr(0, path.find_last_of('/'));

	processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene) {
	// process all the node's meshes (if any)
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene));
	}
	// then do the same for each of its children
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene);
	}
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene) {
	vector<Vertex> vertices;
	vector<unsigned int> indices;
	vector<Material> materials;

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;
		// process vertex positions, normals and texture coordinates
		vertex.position = toVec3(mesh->mVertices[i]);
		vertex.normal = toVec3(mesh->mNormals[i]);
		if (mesh->mTextureCoords[0]) {
			vertex.uv.x = mesh->mTextureCoords[0][i].x;
			vertex.uv.y = mesh->mTextureCoords[0][i].y;
		}
		else {
			vertex.uv = glm::vec2(0, 0);
		}
		
		vertices.push_back(vertex);
	}
	// process indices

	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	// process material
	Material mt;
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		aiColor4D diffuseColor;
		aiColor4D specularColor;
		aiColor4D ambientColor;

		mt.diffuse = loadMaterialTextures(material, aiTextureType_DIFFUSE);
		mt.specular = loadMaterialTextures(material, aiTextureType_SPECULAR);
		mt.ambient = loadMaterialTextures(material, aiTextureType_AMBIENT);

		material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
		material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor);
		material->Get(AI_MATKEY_COLOR_AMBIENT, ambientColor);
		mt.diffuseColor = glm::vec4(diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a);
		mt.specularColor = glm::vec4(specularColor.r, specularColor.g, specularColor.b, specularColor.a);
		mt.ambientColor = glm::vec4(ambientColor.r, ambientColor.g, ambientColor.b, ambientColor.a);
		if (AI_FAILURE == material->Get(AI_MATKEY_SHININESS, mt.shininess)) {
			mt.shininess = 32;
		}
	}

	return Mesh(move(vertices), move(indices), mt);
}

vector<GLuint> Model::loadMaterialTextures(aiMaterial* material, aiTextureType type) {
	vector<GLuint> textures;
	for (unsigned int i = 0; i < material->GetTextureCount(type); i++)
	{
		aiString str;
		material->GetTexture(type, i, &str);
		string path = str.C_Str();
		GLuint texture = textureFromFile(path.c_str(), directory);
		if (texture != -1) {
			textures.push_back(texture);
		}
	}
	return textures;
}