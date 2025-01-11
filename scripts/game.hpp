#ifndef GAME_H
#define GAME_H
constexpr float SPAWN_AMOUNT_MIN = 1;
constexpr float SPAWN_AMOUNT_MAX = 5;
constexpr float SPAWN_COOLDOWN_MIN = 0.5;
constexpr float SPAWN_COOLDOWN_MAX = 3;
constexpr float SPAWN_X_ROT_MIN = 0;
constexpr float SPAWN_X_ROT_MAX = 180;
constexpr float SPAWN_Y_ROT_MIN = 0;
constexpr float SPAWN_Y_ROT_MAX = 180;
constexpr float SPAWN_Z_ROT_MIN = 0;
constexpr float SPAWN_Z_ROT_MAX = 180;
constexpr int MISS_TOLERENCE = 3;

void gameStep();
void initGame();
#endif