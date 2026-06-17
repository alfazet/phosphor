#include "app.hpp"

void framebuffer_size_callback(GLFWwindow *window, i32 newWidth, i32 newHeight) {
    // TODO
}

App::App() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "phosphor", nullptr, nullptr);
    if (window == nullptr) {
        throw std::runtime_error("glfwCreateWindow");
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    if (!gladLoadGL(glfwGetProcAddress)) {
        throw std::runtime_error("gladLoadGL");
    }
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

App::~App() { glfwTerminate(); }

void App::run() {
    while (!glfwWindowShouldClose(window)) {
        glClearColor(1.0, 0.25, 0.5, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        handle_input();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void App::handle_input() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}
