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
		-1, -1, 0, 0, 0,
		-1, 1, 0, 0, 1, 
		1, -1, 0, 1, 0, 
		1, 1, 0, 1, 1
	};

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	vector<unsigned int> indices = { 0, 1, 2, 3, 2, 1 };

	float totalWidth = 0;
	for (char c : text) {
		Character ch = getChar(textSize, c);
		totalWidth += ch.advance;
		characters.push_back(ch);
	}

	float begin = -1;

	for (auto& c : characters) { 
		int vSize = vertices.size() / 5;
		float width = c.size.x;
		float left = begin;
		float right = left + 2 * c.size.x / totalWidth;

		// Top Left
		vertices.push_back(left);
		vertices.push_back(1);
		vertices.push_back(1);
		vertices.push_back(c.uvBottomLeft.x);
		vertices.push_back(c.uvBottomLeft.y + c.uvOffset.y);

		// Top Right
		vertices.push_back(right);
		vertices.push_back(1);
		vertices.push_back(1);
		vertices.push_back(c.uvBottomLeft.x + c.uvOffset.x);
		vertices.push_back(c.uvBottomLeft.y + c.uvOffset.y);

		// Bottom Left
		vertices.push_back(left);
		vertices.push_back(-1);
		vertices.push_back(1);
		vertices.push_back(c.uvBottomLeft.x);
		vertices.push_back(c.uvBottomLeft.y);

		// Bottom Right
		vertices.push_back(right);
		vertices.push_back(-1);
		vertices.push_back(1);
		vertices.push_back(c.uvBottomLeft.x + c.uvOffset.x);
		vertices.push_back(c.uvBottomLeft.y);

		indices.push_back(vSize);
		indices.push_back(vSize + 1);
		indices.push_back(vSize + 2);
		indices.push_back(vSize + 3);
		indices.push_back(vSize + 2);
		indices.push_back(vSize + 1);

		begin += 2 * c.advance / totalWidth + 0.05;
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), indices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)sizeof(glm::vec3));

	glBindVertexArray(0);
}

void UI::Draw(Shader& shader) {
	glActiveTexture(GL_TEXTURE0);
	shader.SetInt("image", 0);
	glBindTexture(GL_TEXTURE_2D, texture);

	if (characters.size() > 0) {
		glActiveTexture(GL_TEXTURE0 + 1);
		shader.SetInt("textAtlas", 1);
		glBindTexture(GL_TEXTURE_2D, characters[0].texture);
	}
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, "transform"), 1, GL_FALSE, glm::value_ptr(transform.matrix));
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6 + 6 * characters.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}