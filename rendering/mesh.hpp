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

struct Texture {
	GLuint id;
	std::string path;
};

struct Material {
	glm::vec4 diffuseColor;
	glm::vec4 specularColor;
	glm::vec4 ambientColor;
	std::vector<Texture> diffuse;
	std::vector<Texture> specular;
	std::vector<Texture> ambient;
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
	void Draw(Shader& shader);
};

Texture textureFromFile(const char* path, std::string directory);
#endif