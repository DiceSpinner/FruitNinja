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
#include "state/window.hpp"
#include "state/state.hpp"
#include "rendering/shader.hpp"
#include "core/object.hpp"
#include "components/rigidbody.hpp"
#include "components/fruit.hpp"
#include "core/ui.hpp"
#include "core/ui3D.hpp"
#include "settings/fruitsize.hpp"
#include "scripts/input.hpp"
#include "scripts/game.hpp"
#include "scripts/frontUI.hpp"
#include "scripts/backUI.hpp"
#include "scripts/initialization.hpp"

using namespace std;
using namespace Game;

const char* objVertexPath = "shaders/object.vert";
const char* objFragPath = "shaders/object.frag";
const char* unlitFrag = "shaders/object_unlit.frag";
const char* ui3DVertPath = "shaders/ui3D.vert";
const char* uiVertPath = "shaders/ui.vert";
const char* uiFragPath = "shaders/ui.frag";

const char* unitCube = "models/unit_cube.obj";
const char* unitSphere = "models/unit_sphere.obj";

int main() {
    initializeContext();
    Shader objectShader(objVertexPath, objFragPath);
    Shader unlitShader(objVertexPath, unlitFrag);
    Shader uiShader(uiVertPath, uiFragPath);
    // Shader rayTracerShader(ui3DVertPath, uiFragPath);

    std::shared_ptr<Model> unitSphereModel = std::make_shared<Model>(unitSphere);
    shared_ptr<Object> sphere = make_shared<Object>(unitSphereModel);
    sphere->transform.SetScale(glm::vec3(WATERMELON_SIZE, WATERMELON_SIZE, WATERMELON_SIZE));

    GLuint smileFace = textureFromFile("awesomeface.png", "images");
    
    UI3D rayIndicator(smileFace);
    rayIndicator.transform.SetScale(glm::vec3(1, 1, 1));
    
    glm::vec3 lightPosition = glm::vec3(0, 0, 0);
    glm::vec4 lightDiffuse(1, 1, 1, 1);
    glm::vec4 lightSpecular(0.5, 0.5, 0.5, 1);
    glm::vec4 lightAmbient(.1, 0.1, 0.1, 1);

    glBindVertexArray(0);

    initGame();
    initFrontUI();
    initBackUI();
    vector<shared_ptr<Object>> objects = {};
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        updateTime();
        processInput(window);

        cameraFront.x = glm::cos(glm::radians(pitch)) * glm::sin(glm::radians(yaw));
        cameraFront.y = glm::sin(glm::radians(pitch));
        cameraFront.z = -glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
        cameraFront = glm::normalize(cameraFront);

        view = glm::lookAt(
            cameraPos,
            cameraPos + cameraFront,
            cameraUp
        );

        setViewProjection(view, perspective);

        gameStep();
        
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        uiShader.Use();
        glUniformMatrix4fv(glGetUniformLocation(uiShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(ortho));
        drawBackUI(uiShader);

        glClear(GL_DEPTH_BUFFER_BIT);
        glm::vec3 lightPos = glm::vec3(0, 0, 10);

        glEnable(GL_DEPTH_TEST);
        unlitShader.Use();
        glUniformMatrix4fv(glGetUniformLocation(unlitShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(unlitShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(perspective));
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
        uiShader.Use();
        drawFrontUI(uiShader);

        /*rayTracerShader.Use();
        glUniformMatrix4fv(glGetUniformLocation(rayTracerShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(rayTracerShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        rayIndicator.transform.SetPosition(cameraPos + 3.0f * getCursorRay());
        rayIndicator.transform.LookAt(cameraPos);*/
        // rayIndicator.Draw(rayTracerShader);
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}