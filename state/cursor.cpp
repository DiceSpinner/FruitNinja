#include <iostream>
#include "cursor.hpp"
#include "window.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

using namespace std;

namespace Game {
	bool mouseClicked = false;
	vector<function<void(glm::vec2)>> OnMousePositionUpdated;
}

static glm::mat4 inverseViewProjection(1);
static glm::mat4 inverseProjection(1);
static glm::mat4 inverseView(1);

void setViewProjection(glm::mat4 view, glm::mat4 projection) {
	inverseView = glm::inverse(view);
	inverseProjection = glm::inverse(projection);
	inverseViewProjection = inverseView * inverseProjection;
}

static glm::vec2 cursorPosDelta(0, 0);
static glm::vec3 cursorRay(0, 0, 1);
static glm::vec2 currPosition(0, 0);

void updateCursorPosition(glm::vec2 position) {
	float halfW = static_cast<float>(SCR_WIDTH) / 2;
	float halfH = static_cast<float>(SCR_HEIGHT) / 2;
	position.x = (position.x - halfW) / halfW;
	position.y = (halfH - position.y) / halfH;

	cursorPosDelta = position - currPosition;
	currPosition = position;

	glm::vec4 viewRay = inverseProjection * glm::vec4(position, 0, 1);
	viewRay.w = 0;
	cursorRay = glm::normalize(glm::vec3(inverseView * viewRay));
	for (auto& observer : Game::OnMousePositionUpdated) {
		observer(position);
	}
}

glm::vec3 getCursorRay() {
	return cursorRay;
}

glm::vec2 getCursorPosDelta() {
	return cursorPosDelta;
}