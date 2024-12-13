#include "window.hpp"
#include <iostream>
#include <ostream>
#include <stdexcept>

static void on_window_size_change(GLFWwindow* window, int width, int height);
static void error_callback(int error, const char* description);

static bool cursor_is_grabbed = false;

/**
 * \brief make a glfw window
 *
 * \details create a glfw window for opengl3.3 core and load in opengl function
 * pointers (implementation)
 *
 * \param screen_width the requested screen height
 * \param screen_height the requested screen width
 * \return an optional window, based on whether or not initialization was
 * successful
 *
 * \author cuppajoeman
 * \date created: 2024-02-25, edited: 2024-07-11
 */
GLFWwindow* initialize_glfw_glad_and_return_window(unsigned int& window_width_px, unsigned int& window_height_px,
    const char* window_name, bool start_in_fullscreen,
    bool start_with_mouse_captured, bool vsync) {

    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        throw std::runtime_error("glfw couldn't be initialized");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;

    if (start_in_fullscreen) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        window_width_px = mode->width;
        window_height_px = mode->height;
        window = glfwCreateWindow(window_width_px, window_height_px, window_name, monitor, NULL);
    }
    else {
        window = glfwCreateWindow(window_width_px, window_height_px, window_name, NULL, NULL);
    }

    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        throw std::runtime_error("failed to create window");
    }

    glfwMakeContextCurrent(window);

    // glad: load all OpenGL function pointers, note that opengl will not work
    // until the next line gets called.

    // glad2
    // if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
    //     std::cout << "Failed to initialize GLAD" << std::endl;
    //     throw std::runtime_error("failed to initialize GLAD");
    // }

    // glad1
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        throw std::runtime_error("failed to initialize GLAD");
    }

    // disable this for debugging so you can move the mouse outside the window
    if (start_with_mouse_captured) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    bool rendering_3d_graphics = true;

    if (rendering_3d_graphics) {
        glEnable(GL_DEPTH_TEST); // configure global opengl state
    }

    int vsync_int = vsync;

    glfwSwapInterval(vsync_int);

    if (glfwRawMouseMotionSupported()) {
        // logger.info("raw mouse motion supported, using it");
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    // NOTE for the other callbacks you have to set them yourself.
    glfwSetFramebufferSizeCallback(window, on_window_size_change);

    return window;
}

static void error_callback(int error, const char* description) { fprintf(stderr, "Error: %s\n", description); }

void toggle_mouse_mode(GLFWwindow* window) {
    if (cursor_is_grabbed) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    cursor_is_grabbed = !cursor_is_grabbed;
}

// glfw: whenever the window size changed (by OS or user resize) this callback
// function executes
void on_window_size_change(GLFWwindow* window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width
    // and height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}