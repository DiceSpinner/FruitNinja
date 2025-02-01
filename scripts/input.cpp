#include "../core/input.hpp"
#include "../components/camera.hpp"
#include "../state/cursor.hpp"
#include "../state/time.hpp"

using namespace Game;

static double pitch = 0;
static double yaw = 0;
static auto cameraPos = glm::vec3(0, 0, 30);
static auto cameraFront = glm::vec3(0, 0, -1);
static auto cameraUp = glm::vec3(0, 1, 0);
static float cameraSpeed = 4;
bool lockedCamera = true;

void onKeyPressed(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
        glfwSetWindowShouldClose(window, true);
    }
    if (key == GLFW_KEY_R && action == GLFW_RELEASE) {
        lockedCamera = !lockedCamera;
        if (lockedCamera) {
            auto cameraPos = glm::vec3(0, 0, 30);
            auto cameraFront = glm::vec3(0, 0, -1);
            auto cameraUp = glm::vec3(0, 1, 0);

            Camera::main->transform.LookAt(cameraPos + cameraFront, cameraUp);
            
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

static double lastCursorX = 0;
static double lastCursorY = 0;
static const double mouseSensitivity = 0.1;
void cursorAim(GLFWwindow* window, double xpos, double ypos) {
    static bool init = true;
    double offsetX = xpos - lastCursorX;
    double offsetY = ypos - lastCursorY;
    lastCursorX = xpos;
    lastCursorY = ypos;

    updateCursorPosition(glm::vec2(xpos, ypos));

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

void processInput(GLFWwindow* window)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        mouseClicked = true;
    }
    else {
        mouseClicked = false;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * deltaTime() * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * deltaTime() * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * deltaTime() * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * deltaTime() * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraSpeed * deltaTime() * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= cameraSpeed * deltaTime() * cameraUp;
}