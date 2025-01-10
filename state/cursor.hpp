#ifndef CURSOR_H
#define CURSOR_H
#include "glm/glm.hpp"

namespace Game {
	extern bool mouseClicked;
}
void setViewProjection(glm::mat4 view, glm::mat4 projection);
void updateCursorPosition(glm::vec2 position);
glm::vec3 getCursorRay();
glm::vec2 getCursorPosDelta();
#endif