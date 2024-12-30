#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include "state/time.hpp"
#include "rendering/shader.hpp"
#include "libraries/stb_image.h"
#include "core/object.hpp"
#include "components/rigidbody.hpp"
#include "fruit.hpp"

using namespace std;

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const char* objVertexPath = "shaders/object.vert";
const char* objFragPath = "shaders/object.frag";
const char* unlitFrag = "shaders/object_unlit.frag";

double pitch = 0;
double yaw = 0;
float cameraSpeed = 2;
glm::vec3 cameraPos(0, 0, 3);
glm::vec3 cameraFront(0, 0, -1);
glm::vec3 cameraUp(0, 1, 0);

static void processInput(GLFWwindow* window)
{
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

static void terminate(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
        glfwSetWindowShouldClose(window, true);
    }
}

const double mouseSensitivity = 0.1;

static void cursorAim(GLFWwindow* window, double xpos, double ypos) {
    static double lastCursorX = 0;
    static double lastCursorY = 0;
    static bool init = true;

    double offsetX = xpos - lastCursorX;
    double offsetY = ypos - lastCursorY;
    lastCursorX = xpos;
    lastCursorY = ypos;

    if (init) {
        init = false;
        return;
    }

    offsetX *= mouseSensitivity;
    offsetY *= mouseSensitivity;

    pitch -= offsetY;
    yaw += offsetX;
    
    pitch = glm::clamp(pitch, -80.0, 80.0);
}

static void onWindowResize(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

static void APIENTRY glDebugOutput(GLenum source,
    GLenum type,
    unsigned int id,
    GLenum severity,
    GLsizei length,
    const char* message,
    const void* userParam)
{
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;

    switch (source)
    {
    case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
    case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
    case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
    case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
    case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
    case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
    case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
    case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
    exit(-1);
}

static GLFWwindow* initializeContext() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef _DEBUG 
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Fruit Ninja", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(-1);
    }
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    glfwSetFramebufferSizeCallback(window, onWindowResize);
    glfwSetKeyCallback(window, terminate);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, cursorAim);
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        // initialize debug output 
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }
    glfwSwapInterval(0);
    return window;
}

const char* backpack = "models/backpack/backpack.obj";
const char* watermelon = "models/fruits/watermelon.obj";
const char* pineapple = "models/fruits/pineapple.obj";

int main() {
    GLFWwindow* window = initializeContext();
    Shader objectShader(objVertexPath, objFragPath);
    Shader unlitShader(objVertexPath, unlitFrag);

    std::shared_ptr<Model> melonModel = std::make_shared<Model>(watermelon);
    std::shared_ptr<Model> backpackModel = std::make_shared<Model>(backpack);
    shared_ptr<Fruit> melon = make_shared<Fruit>(melonModel);
    melon->transform.SetScale(glm::vec3(3, 3, 3));

    shared_ptr<Object> bp = make_shared<Object>(backpackModel);
    bp->AddComponent<Rigidbody>();
    bp->transform.SetPosition(glm::vec3(0, 20, 0));
    
    glm::vec3 lightPosition = glm::vec3(0, 0, 0);
    glm::vec4 lightDiffuse(1, 1, 1, 1);
    glm::vec4 lightSpecular(0.5, 0.5, 0.5, 1);
    glm::vec4 lightAmbient(.1, 0.1, 0.1, 1);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);

    vector<shared_ptr<Object>> objects = {bp, melon};

    while (!glfwWindowShouldClose(window))
    {
        updateTime();
        processInput(window);
        // std::cout << yaw << ", " << pitch << "\n";
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        cameraFront.x = glm::cos(glm::radians(pitch)) * glm::sin(glm::radians(yaw));
        cameraFront.y = glm::sin(glm::radians(pitch));
        cameraFront.z = -glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
        cameraFront = glm::normalize(cameraFront);

        glm::mat4 view = glm::lookAt(
            cameraPos,
            cameraPos + cameraFront,
            cameraUp
        );

        glm::mat4 rotation = glm::rotate(glm::mat4(1), (float)glfwGetTime() / 2, glm::vec3(0, 1, 0));
        glm::vec4 rotatedForward = rotation * glm::vec4(0, 0, 500, 1);
        glm::mat4 lightTransform = glm::translate(glm::mat4(1), glm::vec3(rotatedForward));
        glm::vec3 lightPos = glm::vec3(0, 0, 10);

        unlitShader.Use();
        glUniformMatrix4fv(glGetUniformLocation(unlitShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(unlitShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(unlitShader.ID, "light.position"), 1, glm::value_ptr(lightPos));
        glUniform4fv(glGetUniformLocation(unlitShader.ID, "light.diffuse"), 1, glm::value_ptr(lightDiffuse));
        glUniform4fv(glGetUniformLocation(unlitShader.ID, "light.ambient"), 1, glm::value_ptr(lightAmbient));
        glUniform4fv(glGetUniformLocation(unlitShader.ID, "light.specular"), 1, glm::value_ptr(lightSpecular));
        glUniform3fv(glGetUniformLocation(unlitShader.ID, "cameraPosition"), 1, glm::value_ptr(cameraPos));

        for (auto i = objects.begin(); i != objects.end();) {
            auto& obj = *i;
            obj->Update();
            if (obj->isAlive()) {
                obj->Draw(unlitShader);
                i++;
            }
            else {
                i = objects.erase(i);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        auto error = glGetError();
        if (error) {
            std::cout << "Error: " << error << "\n";
            return -1;
        }
    }

    glfwTerminate();
    return 0;
}