#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include "core/context.hpp"
#include "core/object.hpp"
#include "core/input.hpp"
#include "state/time.hpp"
#include "state/cursor.hpp"
#include "state/window.hpp"
#include "state/state.hpp"
#include "rendering/shader.hpp"
#include "audio/audiosource.hpp"
#include "rendering/camera.hpp"
#include "rendering/renderer.hpp"
#include "rendering/particle_system.hpp"
#include "physics/rigidbody.hpp"
#include "game/fruit.hpp"
#include "core/ui.hpp"
#include "settings/fruitsize.hpp"
#include "game/game.hpp"
#include "game/frontUI.hpp"
#include "game/backUI.hpp"

using namespace std;
using namespace Game;

const char* objVertexPath = "shaders/object.vert";
const char* objFragPath = "shaders/object.frag";
const char* unlitFrag = "shaders/object_unlit.frag";
const char* ui3DVertPath = "shaders/ui3D.vert";
const char* uiVertPath = "shaders/ui.vert";
const char* uiFragPath = "shaders/ui.frag";
const char* particleVertPath = "shaders/particle.vert";
const char* particleFragPath = "shaders/particle.frag";

const char* unitCube = "models/unit_cube.obj";
const char* unitSphere = "models/unit_sphere.obj";

int main() {
    initContext();
    Shader objectShader(objVertexPath, objFragPath);
    Shader unlitShader(objVertexPath, unlitFrag);
    Shader uiShader(uiVertPath, uiFragPath);
    Shader particleShader(particleVertPath, particleFragPath);
    // Shader rayTracerShader(ui3DVertPath, uiFragPath);

    std::shared_ptr<Model> unitSphereModel = std::make_shared<Model>(unitSphere);
    shared_ptr<Object> sphere = Object::Create();
    sphere->SetEnable(false);
    sphere->AddComponent<Renderer>(unitSphereModel);
    sphere->transform.SetScale(glm::vec3(WATERMELON_SIZE, WATERMELON_SIZE, WATERMELON_SIZE));
    
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
        
        glClearColor(0.4f, 0.2f, 0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        uiShader.Use();
        glUniformMatrix4fv(glGetUniformLocation(uiShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(Camera::main->Ortho()));
        drawBackUI(uiShader);

        Object::ActivateNewlyEnabledObjects();

        if (checkShouldPhysicsUpdate()) {
            Object::ExecuteEarlyFixedUpdate();
            Object::ExecuteFixedUpdate();
        }

        Object::ExecuteUpdate();
        AudioSource::DeleteFinishedSources();

        gameStep();

        glEnable(GL_DEPTH_TEST);
        unlitShader.Use();
        glm::vec3 lightPos = glm::vec3(0, 0, 10);
        glUniformMatrix4fv(glGetUniformLocation(unlitShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(Camera::main->View()));
        glUniformMatrix4fv(glGetUniformLocation(unlitShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(Camera::main->Perspective()));
        glUniform3fv(glGetUniformLocation(unlitShader.ID, "light.position"), 1, glm::value_ptr(lightPos));
        glUniform4fv(glGetUniformLocation(unlitShader.ID, "light.diffuse"), 1, glm::value_ptr(lightDiffuse));
        glUniform4fv(glGetUniformLocation(unlitShader.ID, "light.ambient"), 1, glm::value_ptr(lightAmbient));
        glUniform4fv(glGetUniformLocation(unlitShader.ID, "light.specular"), 1, glm::value_ptr(lightSpecular));
        glUniform3fv(glGetUniformLocation(unlitShader.ID, "cameraPosition"), 1, glm::value_ptr(Camera::main->transform.position()));
        Renderer::DrawObjects(unlitShader);

        glDisable(GL_DEPTH_TEST);
        if (lockedCamera) {
            // sphere->GetComponent<Renderer>()->Draw(unlitShader);
        }

        particleShader.Use();
        glUniformMatrix4fv(glGetUniformLocation(particleShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(Camera::main->View()));
        glUniformMatrix4fv(glGetUniformLocation(particleShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(Camera::main->Perspective()));
        ParticleSystem::DrawParticles(particleShader);

        uiShader.Use();
        drawFrontUI(uiShader);

        glfwSwapBuffers(window);

        processInput(window);
    }
    destroyContext();
    return 0;
}