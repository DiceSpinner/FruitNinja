#include <iostream>
#include <unordered_map>
#include <string>
#include "mesh.hpp"
#include "stb_image.h"

using namespace std;

GLuint textureFromFile(const char* path, string directory) {
	static unordered_map<string, GLuint> loadedTextures;
	string filename = string(path);
	filename = directory + '/' + filename;

	auto item = loadedTextures.find(filename);
	if (item != loadedTextures.end()) {
		return item->second;
	}

	GLuint texture;
	glGenTextures(1, &texture);
	stbi_set_flip_vertically_on_load(true);
	int width, height, nrComponents;
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format = GL_RED;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
		loadedTextures.emplace(filename, texture);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
		texture = -1;
	}
	return texture;
}

Mesh::Mesh(vector<Vertex> vertices, vector<unsigned int> indices, Material material) :
	vertices(vertices),
	indices(indices),
	material(material)
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), &indices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

	glBindVertexArray(0);
}

void Mesh::Draw(Shader& shader) {
	int textureIndex = 0;
	for (unsigned int i = 0; i < material.diffuse.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + textureIndex); 
		shader.SetInt(("material.diffuse[" + to_string(i) + "]").c_str(), textureIndex++);
		glBindTexture(GL_TEXTURE_2D, material.diffuse[i]);
	}
	shader.SetVec4("material.diffuseColor", material.diffuseColor);
	shader.SetInt("material.dArraySize", material.diffuse.size());

	for (unsigned int i = 0; i < material.specular.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + textureIndex);
		shader.SetInt(("material.specular[" + to_string(i) + "]").c_str(), textureIndex++);
		glBindTexture(GL_TEXTURE_2D, material.diffuse[i]);
	}
	shader.SetVec4("material.specularColor", material.specularColor);
	shader.SetInt("material.sArraySize", material.specular.size());
	shader.SetFloat("material.shininess", material.shininess);

	for (unsigned int i = 0; i < material.ambient.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + textureIndex);
		shader.SetInt(("material.ambient[" + to_string(i) + "]").c_str(), textureIndex++);
		glBindTexture(GL_TEXTURE_2D, material.ambient[i]);
	}
	shader.SetVec4("material.ambientColor", material.ambientColor);
	shader.SetInt("material.aArraySize", material.ambient.size());
	glActiveTexture(GL_TEXTURE0);

	// draw mesh
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}