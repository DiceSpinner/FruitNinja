#ifndef UI3D_H
#define UI3D_H
#include "glm/glm.hpp"
#include "../rendering/mesh.hpp"
#include "transform.hpp"

class UI3D {
private:
	GLuint texture;
	GLuint VAO, VBO, EBO;
public:
	UI3D(GLuint texture);
	Transform transform;

	void Draw(Shader& shader);
};

#endif