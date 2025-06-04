#include <iostream>
#include "rendering/render_context.hpp"
#include "rendering/shader.hpp"
#include "networking/networking.hpp"
#include "state/time.hpp"
#include "default_state.hpp"

const char* objVertexPath = SHADER_DIR "object.vert";
const char* objFragPath = SHADER_DIR "object.frag";

int main() {
    RenderContext context({ 1920, 1080 });
    Shader objectShader(objVertexPath, objFragPath);

    Networking::init();

    Server server(
        TimeoutSetting{
            .connectionTimeout = std::chrono::seconds(5),
            .connectionRetryInterval = std::chrono::milliseconds(300),
            .requestTimeout = std::chrono::seconds(3),
            .requestRetryInterval = std::chrono::milliseconds(300),
            .impRetryInterval = std::chrono::milliseconds(200)
        }
     );
    std::chrono::steady_clock::duration tickInterval(std::chrono::milliseconds(10));
    std::chrono::steady_clock::time_point tickTime = std::chrono::steady_clock::now();
    while (server.running) {
        server.ProcessInput();
        server.Step();
        server.BroadCastState();
        
        tickTime += tickInterval;
        std::this_thread::sleep_until(tickTime);
    }
    
    Networking::destroy();
}