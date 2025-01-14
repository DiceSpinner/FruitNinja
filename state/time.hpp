#ifndef TIME_H
#define TIME_H
constexpr int PHYSICS_FPS = 60;

void updateTime();
float deltaTime();
float fixedDeltaTime();
float time();
bool checkShouldPhysicsUpdate();
#endif