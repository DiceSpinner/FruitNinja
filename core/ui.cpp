#include "ui.hpp"
#include "glm/ext.hpp"
#include "../state/window.hpp"

using namespace std;

UI::UI(GLuint texture, string text, int textSize) : texture(texture), transform(), textSize(textSize), textureSize(0), isEnabled(true) {
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	vector<float> vertices;
	vector<unsigned int> indices;
	if (texture != -1) {
		glBindTexture(GL_TEXTURE_2D, texture);
		int textureWidth, textureHeight;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &textureWidth);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &textureHeight);

		textureSize.x = textureWidth;
		textureSize.y = textureHeight;

		int flooredHalfWidth = textureWidth / 2;
		int flooredHalfHeight = textureHeight / 2;

		vertices = {
			static_cast<float>(-flooredHalfWidth), static_cast<float>(-flooredHalfHeight), 0, 0, 0,
			static_cast<float>(-flooredHalfWidth), static_cast<float>(flooredHalfHeight), 0, 0, 1,
			static_cast<float>(flooredHalfHeight), static_cast<float>(-flooredHalfHeight), 0, 1, 0,
			static_cast<float>(flooredHalfHeight), static_cast<float>(flooredHalfHeight), 0, 1, 1
		};
		indices = { 0, 1, 2, 3, 2, 1 };
	}

	float totalWidth = 0;
	for (char c : text) {
		Character ch = getChar(textSize, c);
		totalWidth += ch.advance;
		characters.push_back(ch);
	}

	float begin = -totalWidth / 2;

	for (auto& c : characters) { 
		int vSize = vertices.size() / 5;
		float width = c.size.x;
		float left = begin + c.bearing.x;
		float right = left + width;
		float top = c.bearing.y;
		float bottom = c.bearing.y - c.size.y;

		// Top Left
		vertices.push_back(left);
		vertices.push_back(top);
		vertices.push_back(1);
		vertices.push_back(c.uvBottomLeft.x);
		vertices.push_back(c.uvBottomLeft.y + c.uvOffset.y);

		// Top Right
		vertices.push_back(right);
		vertices.push_back(top);
		vertices.push_back(1);
		vertices.push_back(c.uvBottomLeft.x + c.uvOffset.x);
		vertices.push_back(c.uvBottomLeft.y + c.uvOffset.y);

		// Bottom Left
		vertices.push_back(left);
		vertices.push_back(bottom);
		vertices.push_back(1);
		vertices.push_back(c.uvBottomLeft.x);
		vertices.push_back(c.uvBottomLeft.y);

		// Bottom Right
		vertices.push_back(right);
		vertices.push_back(bottom);
		vertices.push_back(1);
		vertices.push_back(c.uvBottomLeft.x + c.uvOffset.x);
		vertices.push_back(c.uvBottomLeft.y);

		indices.push_back(vSize);
		indices.push_back(vSize + 1);
		indices.push_back(vSize + 2);
		indices.push_back(vSize + 3);
		indices.push_back(vSize + 2);
		indices.push_back(vSize + 1);

		begin += c.advance;
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), indices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)sizeof(glm::vec3));

	glBindVertexArray(0);
}

void UI::Draw(Shader& shader) const {
	if (texture != -1) {
		glActiveTexture(GL_TEXTURE0);
		shader.SetInt("image", 0);
		glBindTexture(GL_TEXTURE_2D, texture);
	}

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

void UI::DrawInNDC(glm::vec2 ndc, Shader& shader) {
	float x = ndc.x / 2 * SCR_WIDTH;
	float y = ndc.y / 2 * SCR_HEIGHT;
	transform.SetPosition(glm::vec3(x, y, 0));
	Draw(shader);
}

void UI::UpdateText(string newText) {
	if (newText == displayText) { return; }
	characters.clear();
	vector<float> vertices;
	vector<unsigned int> indices;

	if (texture != -1) {
		int flooredHalfWidth = textureSize.x / 2;
		int flooredHalfHeight = textureSize.y / 2;

		 vertices = {
			static_cast<float>(-flooredHalfWidth), static_cast<float>(-flooredHalfHeight), 0, 0, 0,
			static_cast<float>(-flooredHalfWidth), static_cast<float>(flooredHalfHeight), 0, 0, 1,
			static_cast<float>(flooredHalfHeight), static_cast<float>(-flooredHalfHeight), 0, 1, 0,
			static_cast<float>(flooredHalfHeight), static_cast<float>(flooredHalfHeight), 0, 1, 1
		};
		indices = { 0, 1, 2, 3, 2, 1 };
	}

	float totalWidth = 0;
	for (char c : newText) {
		Character ch = getChar(textSize, c);
		totalWidth += ch.advance;
		characters.push_back(ch);
	}

	float begin = -totalWidth / 2;

	for (auto& c : characters) {
		int vSize = vertices.size() / 5;
		float width = c.size.x;
		float left = begin + c.bearing.x;
		float right = left + width;
		float top = c.bearing.y;
		float bottom = c.bearing.y - c.size.y;

		// Top Left
		vertices.push_back(left);
		vertices.push_back(top);
		vertices.push_back(1);
		vertices.push_back(c.uvBottomLeft.x);
		vertices.push_back(c.uvBottomLeft.y + c.uvOffset.y);

		// Top Right
		vertices.push_back(right);
		vertices.push_back(top);
		vertices.push_back(1);
		vertices.push_back(c.uvBottomLeft.x + c.uvOffset.x);
		vertices.push_back(c.uvBottomLeft.y + c.uvOffset.y);

		// Bottom Left
		vertices.push_back(left);
		vertices.push_back(bottom);
		vertices.push_back(1);
		vertices.push_back(c.uvBottomLeft.x);
		vertices.push_back(c.uvBottomLeft.y);

		// Bottom Right
		vertices.push_back(right);
		vertices.push_back(bottom);
		vertices.push_back(1);
		vertices.push_back(c.uvBottomLeft.x + c.uvOffset.x);
		vertices.push_back(c.uvBottomLeft.y);

		indices.push_back(vSize);
		indices.push_back(vSize + 1);
		indices.push_back(vSize + 2);
		indices.push_back(vSize + 3);
		indices.push_back(vSize + 2);
		indices.push_back(vSize + 1);

		begin += c.advance;
	}
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), indices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)sizeof(glm::vec3));

	glBindVertexArray(0);
}

string UI::text() const {
	return displayText;
}