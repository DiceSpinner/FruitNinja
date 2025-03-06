#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include "core/context.hpp"
#include "core/object.hpp"
#include "core/input.hpp"
#include "state/time.hpp"
#include "state/window.hpp"
#include "rendering/shader.hpp"
#include "rendering/camera.hpp"
#include "rendering/renderer.hpp"
#include "rendering/particle_system.hpp"
#include "game/game.hpp"
#include "game/classic.hpp"

using namespace std;

const char* objVertexPath = "shaders/object.vert";
const char* objFragPath = "shaders/object.frag";
const char* unlitFrag = "shaders/object_unlit.frag";
const char* ui3DVertPath = "shaders/ui3D.vert";
const char* uiVertPath = "shaders/ui.vert";
const char* uiFragPath = "shaders/ui.frag";
const char* particleVertPath = "shaders/particle.vert";
const char* particleFragPath = "shaders/particle.frag";
const char* outlineVertPath = "shaders/outline.vert";
const char* outlineFragPath = "shaders/outline.frag";

int main() {
    initContext();
    Shader objectShader(objVertexPath, objFragPath);
    Shader unlitShader(objVertexPath, unlitFrag);
    Shader uiShader(uiVertPath, uiFragPath);
    Shader particleShader(particleVertPath, particleFragPath);
    Shader outlineShader(outlineVertPath, outlineFragPath);
    Shader vfxShader("shaders/simple.vert", "shaders/simple.frag");
    
    glm::vec3 lightPosition = glm::vec3(0, 0, 0);
    glm::vec4 lightDiffuse(1, 1, 1, 1);
    glm::vec4 lightSpecular(0.5, 0.5, 0.5, 1);
    glm::vec4 lightAmbient(.1, 0.1, 0.1, 1);

    glBindVertexArray(0);

    Game game;
    game.Init();
    game.AddGameModes<ClassicMode>();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        Time::updateTime();
        
        glClearColor(0.4f, 0.2f, 0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        uiShader.Use();
        glUniformMatrix4fv(glGetUniformLocation(uiShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(Camera::main->Ortho()));
        game.DrawBackUI(uiShader);

        Object::ActivateNewlyEnabledObjects();

        if (Time::checkShouldPhysicsUpdate()) {
            Object::ExecuteEarlyFixedUpdate();
            Object::ExecuteFixedUpdate();
        }

        Object::ExecuteUpdate();

        game.Step();

        glEnable(GL_DEPTH_TEST);
        unlitShader.Use();
        glm::vec3 lightPos = glm::vec3(0, 0, 10);
        glProgramUniformMatrix4fv(outlineShader.ID, glGetUniformLocation(outlineShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(Camera::main->View()));
        glProgramUniformMatrix4fv(outlineShader.ID, glGetUniformLocation(outlineShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(Camera::main->Perspective()));

        glUniformMatrix4fv(glGetUniformLocation(unlitShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(Camera::main->View()));
        glUniformMatrix4fv(glGetUniformLocation(unlitShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(Camera::main->Perspective()));
        glUniform3fv(glGetUniformLocation(unlitShader.ID, "light.position"), 1, glm::value_ptr(lightPos));
        glUniform4fv(glGetUniformLocation(unlitShader.ID, "light.diffuse"), 1, glm::value_ptr(lightDiffuse));
        glUniform4fv(glGetUniformLocation(unlitShader.ID, "light.ambient"), 1, glm::value_ptr(lightAmbient));
        glUniform4fv(glGetUniformLocation(unlitShader.ID, "light.specular"), 1, glm::value_ptr(lightSpecular));
        glUniform3fv(glGetUniformLocation(unlitShader.ID, "cameraPosition"), 1, glm::value_ptr(Camera::main->transform.position()));
        Renderer::DrawObjects(unlitShader, outlineShader);

        glDepthMask(GL_FALSE);
        vfxShader.Use();
        glUniformMatrix4fv(glGetUniformLocation(vfxShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(Camera::main->Perspective()));
        glUniformMatrix4fv(glGetUniformLocation(vfxShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(Camera::main->View()));
        game.DrawVFX(vfxShader);

        particleShader.Use();
        glUniform3fv(glGetUniformLocation(particleShader.ID, "cameraPos"), 1, glm::value_ptr(Camera::main->transform.position()));
        glUniformMatrix4fv(glGetUniformLocation(particleShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(Camera::main->Perspective()));
        glUniformMatrix4fv(glGetUniformLocation(particleShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(Camera::main->View()));
        ParticleSystem::DrawParticles(particleShader);

        glDepthMask(GL_TRUE);
        

        glDisable(GL_DEPTH_TEST);
        uiShader.Use();
        game.DrawFrontUI(uiShader);

        glfwSwapBuffers(window);

        processInput(window);
    }
    destroyContext();
    return 0;
}