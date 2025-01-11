#ifndef FRONT_UI_H
#define FRONT_UI_H
#include "../core/ui.hpp"
constexpr glm::vec2 START_BUTTON_POS(-0.5, -0.5);
constexpr glm::vec2 EXIT_BUTTON_POS(0.5, -0.5);
constexpr glm::vec3 FINAL_SCORE_POS(0, 0, 0);

void initFrontUI();
void drawFrontUI(Shader& shader);
#endif