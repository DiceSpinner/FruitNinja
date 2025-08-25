#include <boost/program_options.hpp>
#include "tracy/Tracy.hpp"
#include "rendering/render_context.hpp"
#include "audio/audio_context.hpp"
#include "rendering/font.hpp"
#include "infrastructure/clock.hpp"
#include "networking/networking.hpp"
#include "infrastructure/object.hpp"
#include "input.hpp"
#include "rendering/shader.hpp"
#include "rendering/camera.hpp"
#include "rendering/renderer.hpp"
#include "rendering/particle_system.hpp"
#include "game/state_selection.hpp"

using namespace std;

const char* objVertexPath = SHADER_DIR "object.vert";
const char* objFragPath = SHADER_DIR "object.frag";
const char* unlitFrag = SHADER_DIR "object_unlit.frag";
const char* ui3DVertPath = SHADER_DIR "ui3D.vert";
const char* uiVertPath = SHADER_DIR "ui.vert";
const char* uiFragPath = SHADER_DIR "ui.frag";
const char* particleVertPath = SHADER_DIR "particle.vert";
const char* particleFragPath = SHADER_DIR "particle.frag";
const char* outlineVertPath = SHADER_DIR "outline.vert";
const char* outlineFragPath = SHADER_DIR "outline.frag";
const char* vfxShaderVertPath = SHADER_DIR "simple.vert";
const char* vfxShaderFragPath = SHADER_DIR "simple.frag";
const uint32_t PHYSICS_FPS = 60;

namespace po = boost::program_options;


int main(int argc, char* argv[]) {
    TracyNoop;
    std::string serverIP;
    USHORT serverPort;
    USHORT localPort;

    po::options_description cmdOptions("Options:");
    cmdOptions.add_options()
        ("help,h", "show help message")
        ("ip_server,ips", po::value<std::string>(&serverIP)->default_value("127.0.0.1"), "Public IP for the server")
        ("port_server,ps", po::value<USHORT>(&serverPort)->default_value(30000), "Port number used by the server")
        ("port,p", po::value<USHORT>(&localPort)->default_value(35000), "Port number used by the client");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, cmdOptions), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << cmdOptions;
        return 0;
    }

    Networking::init();
    RenderContext renderContext({1920, 1080});
    Font::init();

    ObjectManager manager;
    Clock clock(PHYSICS_FPS);
    Audio::initContext(manager);

    Input::initInput(renderContext.Window(), clock);
    Shader objectShader(objVertexPath, objFragPath);
    Shader unlitShader(objVertexPath, unlitFrag);
    Shader uiShader(uiVertPath, uiFragPath);
    Shader particleShader(particleVertPath, particleFragPath);
    Shader outlineShader(outlineVertPath, outlineFragPath);
    Shader vfxShader(vfxShaderVertPath, vfxShaderFragPath);
    
    glm::vec3 lightPosition = glm::vec3(0, 0, 0);
    glm::vec4 lightDiffuse(1, 1, 1, 1);
    glm::vec4 lightSpecular(0.5, 0.5, 0.5, 1);
    glm::vec4 lightAmbient(.1, 0.1, 0.1, 1);

    glBindVertexArray(0);

    sockaddr_in serverAddr = {
        .sin_family = AF_INET,
        .sin_port = htons(serverPort)
    };

    inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

    Game game(manager, clock, localPort, serverAddr);
    game.Init();
    game.SetInitialGameState<SelectionState>();

    while (!glfwWindowShouldClose(renderContext.Window()))
    {
        clock.Tick();
        glfwPollEvents();
        Input::processInput(renderContext.Window());
        
        glClearColor(0.4f, 0.2f, 0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        uiShader.Use();
        glUniformMatrix4fv(glGetUniformLocation(uiShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(Camera::main->Ortho()));
        game.DrawBackUI(uiShader);

        manager.Tick(clock);
        game.Step();

        glEnable(GL_DEPTH_TEST);
        unlitShader.Use();
        glm::vec3 lightPos = glm::vec3(0, 0, 10);
        glProgramUniformMatrix4fv(outlineShader.ID, glGetUniformLocation(outlineShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(Camera::main->View()));
        glProgramUniformMatrix4fv(outlineShader.ID, glGetUniformLocation(outlineShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(Camera::main->Projection()));

        glUniformMatrix4fv(glGetUniformLocation(unlitShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(Camera::main->View()));
        glUniformMatrix4fv(glGetUniformLocation(unlitShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(Camera::main->Projection()));
        glUniform3fv(glGetUniformLocation(unlitShader.ID, "light.position"), 1, glm::value_ptr(lightPos));
        glUniform4fv(glGetUniformLocation(unlitShader.ID, "light.diffuse"), 1, glm::value_ptr(lightDiffuse));
        glUniform4fv(glGetUniformLocation(unlitShader.ID, "light.ambient"), 1, glm::value_ptr(lightAmbient));
        glUniform4fv(glGetUniformLocation(unlitShader.ID, "light.specular"), 1, glm::value_ptr(lightSpecular));
        glUniform3fv(glGetUniformLocation(unlitShader.ID, "cameraPosition"), 1, glm::value_ptr(Camera::main->transform.position()));
        Renderer::DrawObjects(unlitShader, outlineShader);

        glDepthMask(GL_FALSE);
        vfxShader.Use();
        glUniformMatrix4fv(glGetUniformLocation(vfxShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(Camera::main->Projection()));
        glUniformMatrix4fv(glGetUniformLocation(vfxShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(Camera::main->View()));
        game.DrawVFX(vfxShader);

        particleShader.Use();
        glUniform3fv(glGetUniformLocation(particleShader.ID, "cameraPos"), 1, glm::value_ptr(Camera::main->transform.position()));
        glUniformMatrix4fv(glGetUniformLocation(particleShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(Camera::main->Projection()));
        glUniformMatrix4fv(glGetUniformLocation(particleShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(Camera::main->View()));
        ParticleSystem::DrawParticles(particleShader);
        glDepthMask(GL_TRUE);
        
        glDisable(GL_DEPTH_TEST);
        uiShader.Use();
        game.DrawFrontUI(uiShader);

        glfwSwapBuffers(renderContext.Window());
    }

    Font::destroy();
    Audio::destroyContext();
    return 0;
}