#ifndef PHOSPHOR_APP_HPP
#define PHOSPHOR_APP_HPP

#include "common.hpp"

constexpr usize WIN_WIDTH = 1920;
constexpr usize WIN_HEIGHT = 1080;

class App {
  public:
    GLFWwindow *window;

    explicit App();

    ~App();

    void run();

    void handle_input();
};

#endif // PHOSPHOR_APP_HPP
