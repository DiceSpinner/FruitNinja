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
#include "state/cursor.hpp"
#include "state/camera.hpp"
#include "state/new_objects.hpp"
#include "rendering/shader.hpp"
#include "core/object.hpp"
#include "components/rigidbody.hpp"
#include "components/fruit.hpp"
#include "core/ui.hpp"
#include "core/ui3D.hpp"
#include "settings/fruitsize.hpp"

using namespace std;
using namespace GameState;

// settings
static unsigned int SCR_WIDTH = 800;
static unsigned int SCR_HEIGHT = 600;

const char* objVertexPath = "shaders/object.vert";
const char* objFragPath = "shaders/object.frag";
const char* unlitFrag = "shaders/object_unlit.frag";
const char* ui3DVertPath = "shaders/ui3D.vert";
const char* uiVertPath = "shaders/ui.vert";
const char* uiFragPath = "shaders/ui.frag";

double pitch = 0;
double yaw = 0;
float cameraSpeed = 2;

bool lockedCamera = false;

static void processInput(GLFWwindow* window)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        mouseClicked = true;
    }
    else {
        mouseClicked = false;
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * deltaTime() * cameraFront;
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

static void onKeyPressed(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
        glfwSetWindowShouldClose(window, true);
    }
    if (key == GLFW_KEY_R && action == GLFW_RELEASE) {
        lockedCamera = !lockedCamera;
        if (lockedCamera) {
            GameState::cameraPos = glm::vec3(0, 0, 30);
            GameState::cameraFront = glm::vec3(0, 0, -1);
            GameState::cameraUp = glm::vec3(0, 1, 0);
            pitch = 0;
            yaw = 0;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

const double mouseSensitivity = 0.1;

static double lastCursorX = 0;
static double lastCursorY = 0;
static void cursorAim(GLFWwindow* window, double xpos, double ypos) {
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

static void onWindowResize(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
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
    glfwSetKeyCallback(window, onKeyPressed);
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
const char* unitCube = "models/unit_cube.obj";
const char* unitSphere = "models/unit_sphere.obj";

const char* apple = "models/fruits/apple.obj";
const char* appleSlice1 = "models/fruits/apple_top.obj";
const char* appleSlice2 = "models/fruits/apple_bottom.obj";

int main() {
    GLFWwindow* window = initializeContext();
    Shader objectShader(objVertexPath, objFragPath);
    Shader unlitShader(objVertexPath, unlitFrag);
    Shader uiShader(uiVertPath, uiFragPath);
    Shader rayTracerShader(ui3DVertPath, uiFragPath);

    std::shared_ptr<Model> melonModel = std::make_shared<Model>(watermelon);
    std::shared_ptr<Model> pineappleModel = std::make_shared<Model>(pineapple);
    std::shared_ptr<Model> backpackModel = std::make_shared<Model>(backpack);
    std::shared_ptr<Model> unitSphereModel = std::make_shared<Model>(unitSphere);

    std::shared_ptr<Model> appleModel = std::make_shared<Model>(apple);
    std::shared_ptr<Model> appleSlice1Model = std::make_shared<Model>(appleSlice1);
    std::shared_ptr<Model> appleSlice2Model = std::make_shared<Model>(appleSlice2);

    shared_ptr<Object> appleObject = make_shared<Object>(appleModel);
    appleObject->transform.SetScale(glm::vec3(1, 1, 1));
    appleObject->AddComponent<Fruit>(APPLE_SIZE, 1, appleSlice1Model, appleSlice2Model);

    shared_ptr<Object> bp = make_shared<Object>(pineappleModel);
    bp->AddComponent<Rigidbody>();
    bp->transform.SetPosition(glm::vec3(0, 20, 0));

    shared_ptr<Object> sphere = make_shared<Object>(unitSphereModel);
    sphere->transform.SetScale(glm::vec3(APPLE_SIZE, APPLE_SIZE, APPLE_SIZE));

    Texture image = textureFromFile("wood1.jpg", "images");
    Texture smileFace = textureFromFile("awesomeface.png", "images");
    UI background(image);
    UI3D rayIndicator(smileFace);
    rayIndicator.transform.SetScale(glm::vec3(1, 1, 1));
    
    glm::vec3 lightPosition = glm::vec3(0, 0, 0);
    glm::vec4 lightDiffuse(1, 1, 1, 1);
    glm::vec4 lightSpecular(0.5, 0.5, 0.5, 1);
    glm::vec4 lightAmbient(.1, 0.1, 0.1, 1);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 300.0f);

    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);

    vector<shared_ptr<Object>> objects = {bp, appleObject};

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        updateTime();
        processInput(window);

        cameraFront.x = glm::cos(glm::radians(pitch)) * glm::sin(glm::radians(yaw));
        cameraFront.y = glm::sin(glm::radians(pitch));
        cameraFront.z = -glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
        cameraFront = glm::normalize(cameraFront);

        glm::mat4 view = glm::lookAt(
            cameraPos,
            cameraPos + cameraFront,
            cameraUp
        );

        updateCursorPosition(glm::vec2(lastCursorX, lastCursorY), SCR_WIDTH, SCR_HEIGHT);
        setViewProjection(view, projection);
        
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        uiShader.Use();
        background.Draw(uiShader);
        glClear(GL_DEPTH_BUFFER_BIT);
        glm::vec3 lightPos = glm::vec3(0, 0, 10);

        unlitShader.Use();
        glUniformMatrix4fv(glGetUniformLocation(unlitShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(unlitShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(unlitShader.ID, "light.position"), 1, glm::value_ptr(lightPos));
        glUniform4fv(glGetUniformLocation(unlitShader.ID, "light.diffuse"), 1, glm::value_ptr(lightDiffuse));
        glUniform4fv(glGetUniformLocation(unlitShader.ID, "light.ambient"), 1, glm::value_ptr(lightAmbient));
        glUniform4fv(glGetUniformLocation(unlitShader.ID, "light.specular"), 1, glm::value_ptr(lightSpecular));
        glUniform3fv(glGetUniformLocation(unlitShader.ID, "cameraPosition"), 1, glm::value_ptr(cameraPos));

        while (!newObjects.empty()) {
            objects.push_back(newObjects.front());
            newObjects.pop();
        }

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

        glClear(GL_DEPTH_BUFFER_BIT);
        if (lockedCamera) {
            // sphere->Draw(unlitShader);
        }

        glClear(GL_DEPTH_BUFFER_BIT);
        rayTracerShader.Use();
        glUniformMatrix4fv(glGetUniformLocation(rayTracerShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(rayTracerShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        rayIndicator.transform.SetPosition(cameraPos + 50.0f * getCursorRay());
        rayIndicator.transform.LookAt(cameraPos);
        // rayIndicator.Draw(rayTracerShader);
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}