#include "camera.hpp"
namespace Game {
    glm::vec3 cameraPos = glm::vec3(0, 0, 30);
    glm::vec3 cameraFront = glm::vec3(0, 0, -1);
    glm::vec3 cameraUp = glm::vec3(0, 1, 0);
    bool lockedCamera = true;
    double pitch = 0;
    double yaw = 0;
    float cameraSpeed = 2;
}
