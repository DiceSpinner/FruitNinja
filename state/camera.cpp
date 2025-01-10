#include "camera.hpp"
#include "window.hpp"
#include <glm/glm.hpp>
#include <glm/ext.hpp>

namespace Game {
    glm::vec3 cameraPos = glm::vec3(0, 0, 30);
    glm::vec3 cameraFront = glm::vec3(0, 0, -1);
    glm::vec3 cameraUp = glm::vec3(0, 1, 0);
    bool lockedCamera = true;
    double pitch = 0;
    double yaw = 0;
    float cameraSpeed = 4;
    float near;
    float far;
    glm::mat4 ortho(1);
    glm::mat4 perspective(1);
    glm::mat4 view(1);
}

using namespace Game;

void setCameraPerspective(float near, float far) {
    Game::near = near;
    Game::far = far;
    if (SCR_HEIGHT > 0) {
        perspective = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, near, far);
    }
}

float computeNormalizedZ(float viewZ) {
    float top = 2 * near * far - viewZ * (far + near);
    float bot = -viewZ * (far - near);
    return top / bot;
}
