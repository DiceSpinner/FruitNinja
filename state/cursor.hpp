#ifndef CURSOR_H
#define CURSOR_H
#include <vector>
#include <functional>
#include "glm/glm.hpp"

namespace Game {
	extern bool mouseClicked;
	extern std::vector<std::function<void(glm::vec2)>> OnMousePositionUpdated;
}
void setViewProjection(glm::mat4 view, glm::mat4 projection);
void updateCursorPosition(glm::vec2 position);
glm::vec3 getCursorRay();
glm::vec2 getCursorPosDelta();
#endif