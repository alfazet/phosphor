#include "camera.hpp"
#include "common.hpp"
#include "hittable.hpp"
#include "limits"
#include "material.hpp"
#include "pointlight.hpp"
#include "ray.hpp"
#include "scene.hpp"
#include "scenereader.hpp"
#include <ctime>

int main(int argc, char **argv) {
    srand(time(nullptr));

    auto scenes = ReadFile("./models/Box/glTF/Box.gltf");
    auto scene = scenes[0];
    scene.add_camera(Camera(
        vec3(5.0f, 5.0f, 5.0f),
        vec3(0.0f, 0.0f, 0.0f),
        vec3(0.0f, 1.0f, 0.0f),
        45.0f,
        1.0
    ));
    scene.set_camera(0);

    // note to the future people reading this
    // it is expected that the image is black when no light is added
    scene.add_light(PointLight(vec3(2.5f, 3.5f, 3.0f), 500.0f));

    scene.generate_image(512, 50, 10'000, 3);


    return 0;
}
