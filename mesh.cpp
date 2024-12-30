#include "mesh.hpp"

using namespace std;

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
		shader.setInt(("material.diffuse[" + to_string(i) + "]").c_str(), textureIndex++);
		glBindTexture(GL_TEXTURE_2D, material.diffuse[i].id);
	}
	shader.setVec4("material.diffuseColor", material.diffuseColor);
	shader.setInt("material.dArraySize", material.diffuse.size());

	for (unsigned int i = 0; i < material.specular.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + textureIndex);
		shader.setInt(("material.specular[" + to_string(i) + "]").c_str(), textureIndex++);
		glBindTexture(GL_TEXTURE_2D, material.diffuse[i].id);
	}
	shader.setVec4("material.specularColor", material.specularColor);
	shader.setInt("material.sArraySize", material.specular.size());
	shader.setFloat("material.shininess", material.shininess);

	for (unsigned int i = 0; i < material.ambient.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + textureIndex);
		shader.setInt(("material.ambient[" + to_string(i) + "]").c_str(), textureIndex++);
		glBindTexture(GL_TEXTURE_2D, material.ambient[i].id);
	}
	shader.setVec4("material.ambientColor", material.ambientColor);
	shader.setInt("material.aArraySize", material.ambient.size());
	glActiveTexture(GL_TEXTURE0);

	// draw mesh
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}