#include <iostream>
#include "rendering/render_context.hpp"
#include "rendering/shader.hpp"
#include "networking/networking.hpp"
#include "server.hpp"

const char* objVertexPath = SHADER_DIR "object.vert";
const char* objFragPath = SHADER_DIR "object.frag";

const uint32_t FPS = 60;

int main() {
    RenderContext context({ 1920, 1080 });
    Shader objectShader(objVertexPath, objFragPath);

    Networking::init();

    Server server(
        TimeoutSetting{
            .connectionTimeout = std::chrono::seconds(5),
            .connectionRetryInterval = std::chrono::milliseconds(300),
            .impRetryInterval = std::chrono::milliseconds(200),
            .replyKeepDuration = std::chrono::seconds(1)
        }, FPS
     );
    std::chrono::duration<float> tickInterval(1.0 / FPS);

    while (server.running) {
        server.ProcessInput();
        server.Step();
        
        std::this_thread::sleep_for(tickInterval);
    }
    
    Networking::destroy();
}