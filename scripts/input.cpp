#include "input.hpp"
#include "../state/camera.hpp"

using namespace Game;

void onKeyPressed(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
        glfwSetWindowShouldClose(window, true);
    }
    if (key == GLFW_KEY_R && action == GLFW_RELEASE) {
        lockedCamera = !lockedCamera;
        if (lockedCamera) {
            cameraPos = glm::vec3(0, 0, 30);
            cameraFront = glm::vec3(0, 0, -1);
            cameraUp = glm::vec3(0, 1, 0);
            pitch = 0;
            yaw = 0;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

static double lastCursorX = 0;
static double lastCursorY = 0;
void cursorAim(GLFWwindow* window, double xpos, double ypos) {
    static bool init = true;
    double offsetX = xpos - lastCursorX;
    double offsetY = ypos - lastCursorY;
    lastCursorX = xpos;
    lastCursorY = ypos;

    if (lockedCamera) {
        return;
    }

    if (init) {
        init = false;
        return;
    }

    offsetX *= mouseSensitivity;
    offsetY *= mouseSensitivity;

    pitch -= offsetY;
    yaw += offsetX;

    pitch = glm::clamp(pitch, -85.0, 85.0);
}