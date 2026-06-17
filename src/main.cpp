#include "app.hpp"

int main(int argc, char **argv) {
    try {
        App app{};
        app.run();
    } catch (std::exception &e) {
        printf("error %s\n", e.what());
    }

    return 0;
}
