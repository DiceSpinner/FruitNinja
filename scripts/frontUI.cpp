#include "frontUI.hpp"
#include "../state/window.hpp"
#include "../state/state.hpp"

using namespace std;
using namespace Game;

static glm::vec4 scoreColor(1, 1, 0, 1);
static glm::vec4 startColor(0, 1, 0, 1);
static glm::vec4 exitColor(1, 0, 0, 1);
static glm::vec4 restartColor(1, 1, 0, 1);

static UI scoreBoard;
static UI largeScoreBoard;
static UI startButtom;
static UI exitButton;
static UI restartButton;
static UI redCrosses[3];
static UI emptyCrosses[3];

void initFrontUI() {
	scoreBoard = UI(-1, "Score: ");
	largeScoreBoard = UI(-1, "Score", 70);
	startButtom = UI(-1, "Start");
	exitButton = UI(-1, "Exit");
	restartButton = UI(-1, "Restart");
	restartButton.transform.SetPosition(FINAL_SCORE_POS);

	GLuint emptyCross = textureFromFile("empty_cross.png", "images");
	GLuint redCross = textureFromFile("red_cross.png", "images");
	for (auto i = 0; i < 3;i++) {
		redCrosses[i] = UI(redCross);
		emptyCrosses[i] = UI(emptyCross);
		glm::mat4 rotation = glm::rotate(glm::mat4(1), (float)glm::radians(10 + 10.0f * (2 - i)), glm::vec3(0, 0, -1));
		redCrosses[i].transform.SetRotation(rotation);
		redCrosses[i].transform.SetScale(glm::vec3(0.1, 0.1, 0.1));
		emptyCrosses[i].transform.SetRotation(rotation);
		emptyCrosses[i].transform.SetScale(glm::vec3(0.05, 0.05, 0.05));
	}
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
		scoreBoard.transform.SetPosition(glm::vec3(-0.87 * halfWidth, 0.9 * halfHeight, 0));
		scoreBoard.UpdateText("Score: " + to_string(score));
		scoreBoard.Draw(shader);

		for (int i = 0; i < 3;i++) {
			if (i < Game::misses) {
				redCrosses[i].transform.SetPosition(glm::vec3((0.95 - i * 0.06) * halfWidth, (0.8 + (i + 1) * 0.04) * halfHeight, 0));
				redCrosses[i].Draw(shader);
			}
			else {
				emptyCrosses[i].transform.SetPosition(glm::vec3((0.95 - i * 0.06) * halfWidth, (0.8 + (i + 1) * 0.04) * halfHeight, 0));
				emptyCrosses[i].Draw(shader);
			}
		}
	}
	else {
		shader.SetVec4("textColor", restartColor);
		restartButton.DrawInNDC(START_BUTTON_POS, shader);

		shader.SetVec4("textColor", exitColor);
		exitButton.DrawInNDC(EXIT_BUTTON_POS, shader);

		shader.SetVec4("textColor", scoreColor);
		largeScoreBoard.UpdateText("Score: " + to_string(score));
		largeScoreBoard.Draw(shader);
	}
}