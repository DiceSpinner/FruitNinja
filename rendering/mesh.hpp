#ifndef MESH_H
#define MESH_H
#include "glad/glad.h"
#include "glm/glm.hpp"
#include <string>
#include <vector>
#include <variant>
#include "shader.hpp"

enum TextureType {
	Diffuse,
	Specular
};

struct Material {
	glm::vec4 diffuseColor;
	glm::vec4 specularColor;
	glm::vec4 ambientColor;
	std::vector<GLuint> diffuse;
	std::vector<GLuint> specular;
	std::vector<GLuint> ambient;
	float shininess;
};

struct Vertex {
	glm::vec3 position;
	glm::vec2 uv;
	glm::vec3 normal;
};

class Mesh {
private:
	GLuint VBO, EBO;
public:
	GLuint VAO;
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	Material material;
	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, Material material);
	void Destroy() const;
	void Draw(Shader& shader);
};

GLuint textureFromFile(const char* path, std::string directory);
#endif