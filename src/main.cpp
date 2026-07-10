#include "camera.hpp"
#include "common.hpp"
#include "hittable.hpp"
#include "pointlight.hpp"
#include "scene.hpp"
#include "scenereader.hpp"
#include "printers.hpp"
#include <ctime>



int main(int argc, char **argv) {
    srand(time(nullptr));

    // auto scenes = ReadFile("./models/Box/glTF/Box.gltf");
    auto scenes = ReadFile("./models/Duck/glTF/Duck.gltf");
    auto scene = scenes[0];
    vec3 red = vec3(500.0f, 0.0f, 0.0f);
    vec3 green = vec3(0.0f, 500.0f, 0.0f);
    scene.add_light(PointLight(vec3(2.5f, 3.5f, 3.0f), red));
    scene.add_light(PointLight(vec3(2.5f, -3.5f, 3.0f), green));
    scene.add_light(PointLight(vec3(-2.5f, -3.5f, 3.0f), green));
    scene.add_light(PointLight(vec3(-2.5f, 3.5f, 3.0f), red));
    print_camera(scene.get_camera());
    scene.generate_image(256, 50, 10'000, 3);

    return 0;
}

