#ifndef WINDOW_HPP
#define WINDOW_HPP

#include "live_input_state.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

GLFWwindow* initialize_glfw_glad_and_return_window(unsigned int& window_width_px, unsigned int& window_height_px,
    const char* window_name, bool start_in_fullscreen,
    bool start_with_mouse_captured, bool vsync);

#endif // WINDOW_HPP