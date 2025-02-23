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
	GLuint texture = -1;
	GLuint VAO = -1, VBO = -1, EBO = -1;
	std::vector<Character> characters;
	std::string displayText;
	int textSize = 0;
	glm::ivec2 textureSize{0, 0};
public:
	bool isEnabled = true;
	glm::vec4 textColor;
	glm::vec4 imageColor;
	Transform transform;

	UI(GLuint texture, std::string text="", int textSize = 50);
	UI() = default;

	void Draw(Shader& shader) const;
	void DrawInNDC(glm::vec2 ndc, Shader& shader);
	void UpdateText(std::string newText);
	std::string text() const;
};

#endif