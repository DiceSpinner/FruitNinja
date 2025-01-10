#include "frontUI.hpp"
#include "../state/window.hpp"
#include "../state/state.hpp"
#include "../settings/uiSetting.hpp"

using namespace std;
using namespace Game;

static glm::vec4 scoreColor(1, 1, 0, 1);
static glm::vec4 startColor(0, 1, 0, 1);
static glm::vec4 exitColor(1, 0, 0, 1);

static UI scoreBoard;
static UI startButtom;
static UI exitButton;

void initFrontUI() {
	scoreBoard = UI(-1, "Score: ");
	startButtom = UI(-1, "Start");
	exitButton = UI(-1, "Exit");
}

void drawFrontUI(Shader& shader) {
	
	float halfWidth = SCR_WIDTH / 2.0f;
	float halfHeight = SCR_HEIGHT / 2.0f;
	if (Game::state == State::START) {
		shader.SetVec4("textColor", startColor);
		startButtom.DrawInNDC(START_BUTTON_POS, shader);
		shader.SetVec4("textColor", exitColor);
		exitButton.DrawInNDC(EXIT_BUTTON_POS, shader);
	}
	else if (Game::state == State::GAME) {
		shader.SetVec4("textColor", scoreColor);
		scoreBoard.transform.SetPosition(glm::vec3(-halfWidth + 130, halfHeight - 50, 0));
		scoreBoard.UpdateText("Score: " + to_string(score));
		scoreBoard.Draw(shader);
	}
}