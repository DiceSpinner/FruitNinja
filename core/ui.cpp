#include "ui.hpp"
#include "glm/ext.hpp"

using namespace std;

UI::UI(GLuint texture, string text, int textSize) : texture(texture), transform() {
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	vector<float> vertices = {
		0, 0, 0, 0, 0,
		0, 1, 0, 0, 1, 
		1, 0, 0, 1, 0, 
		1, 1, 0, 1, 1
	};

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	vector<unsigned int> indices = { 0, 1, 2, 1, 3, 2 };

	float totalWidth = 0;
	for (char c : text) {
		Character ch = getChar(textSize, c);
		totalWidth += ch.advance;
		characters.push_back(ch);
	}

	float begin = 0;
	for (auto& c : characters) { 
		float width = c.advance;
		float left = begin;
		float right = left + c.advance / totalWidth;
		glm::vec3 topLeft(left, 1, 1);
		glm::vec3 topRight(right, 1, 1);
		glm::vec3 botLeft(left, 0, 1);
		glm::vec3 botRight(right, 0, 1);

		// Top Left
		vertices.push_back(left);
		vertices.push_back(1);
		vertices.push_back(1);
		vertices.push_back(c.uv.x + left);
		vertices.push_back(1 - c.uv.y);

		// 

		begin = right;
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), indices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)sizeof(glm::vec3));

	glBindVertexArray(0);
}

void UI::Draw(Shader& shader) {
	glActiveTexture(GL_TEXTURE0);
	shader.SetInt("image", 0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "transform"), 1, GL_FALSE, glm::value_ptr(transform.matrix));
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}