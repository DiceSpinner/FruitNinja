#include "backUI.hpp"
#include "../state/state.hpp"

static UI background;
static UI backgroundText;

void initBackUI() {
	GLuint image = textureFromFile("fruit_ninja_clean.png", "images");
	background = UI(image);
	backgroundText = UI(0, "Fruit Ninja", 100);
	backgroundText.textColor = glm::vec4(1, 1, 0, 1);
}

void drawBackUI(Shader& shader) {
	background.Draw(shader);
	
	if (Game::state == State::START) {
		backgroundText.Draw(shader);
	}
}