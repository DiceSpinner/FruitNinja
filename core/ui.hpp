#ifndef UI_H
#define UI_H
#include "glm/glm.hpp"
#include "../rendering/mesh.hpp"
#include "transform.hpp"

class UI {
private:
	Texture texture;
	GLuint VAO, VBO, EBO;
public:
	UI(Texture texture);
	Transform transform;

	void Draw(Shader& shader);
};

#endif