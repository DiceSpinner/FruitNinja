#include "frontUI.hpp"
#include "../state/screen.hpp"
#include "../state/score.hpp"

using namespace std;
using namespace GameState;

static glm::vec4 scoreColor(1, 1, 0, 1);

static UI scoreBoard;

void initGameUI() {
	scoreBoard = UI(-1, "Score: ");
}

void drawGameUI(Shader& shader) {
	shader.SetVec4("textColor", scoreColor);
	float halfWidth = SCR_WIDTH / 2.0f;
	float halfHeight = SCR_HEIGHT / 2.0f;
	scoreBoard.transform.SetPosition(glm::vec3(-halfWidth + 110, halfHeight - 50, 0));
	scoreBoard.UpdateText("Score: " + to_string(score));
	scoreBoard.Draw(shader);
}