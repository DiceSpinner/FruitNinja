#ifndef UI_H
#define UI_H
#include <string>
#include <vector>
#include "glm/glm.hpp"
#include "../rendering/font.hpp"
#include "../rendering/mesh.hpp"
#include "transform.hpp"

class UI {
private:
	GLuint texture;
	GLuint VAO, VBO, EBO;
	std::vector<Character> characters;
public:
	UI(GLuint texture, std::string text="", int textSize = 30);
	Transform transform;

	void Draw(Shader& shader);
};

#endif