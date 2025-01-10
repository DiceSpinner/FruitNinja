#ifndef CAMERA_H
#define CAMERA_H
#include "glm/glm.hpp"
namespace Game {
	extern glm::vec3 cameraPos;
	extern glm::vec3 cameraUp;
	extern glm::vec3 cameraFront;
	extern bool lockedCamera;
	extern double pitch;
	extern double yaw;
	extern float cameraSpeed;
}
#endif