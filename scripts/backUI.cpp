#include "backUI.hpp"
#include "../state/state.hpp"

static glm::vec4 textColor(1, 1, 0, 1);
static UI background;
static UI backgroundText;

void initBackUI() {
	GLuint image = textureFromFile("fruit_ninja_clean.png", "images");
	background = UI(image);
	backgroundText = UI(-1, "Fruit Ninja", 100);
}

void drawBackUI(Shader& shader) {
	background.Draw(shader);
	
	if (Game::state == State::START) {
		shader.SetVec4("textColor", textColor);
		backgroundText.Draw(shader);
	}
}