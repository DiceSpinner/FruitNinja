#ifndef FRONT_UI_H
#define FRONT_UI_H
#include "../core/ui.hpp"
constexpr glm::vec2 START_BUTTON_POS(-0.5, -0.5);
constexpr glm::vec2 EXIT_BUTTON_POS(0.5, -0.5);
constexpr glm::vec3 FINAL_SCORE_POS(0, 0, 0);
constexpr size_t MAX_MOUSE_POS_SIZE = 100;
constexpr size_t MAX_TRAIL_SIZE = 20;
constexpr float TRAIL_ARROW_SIZE = 50;
constexpr float POS_KEEP_TIME = 0.3f;

void initFrontUI();
void drawFrontUI(Shader& shader);
#endif