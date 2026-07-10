#include "camera.hpp"
#include "common.hpp"
#include "hittable.hpp"
#include "pointlight.hpp"
#include "scene.hpp"
#include "scenereader.hpp"
#include "printers.hpp"

#include <cstring>
#include <ctime>


void print_usage() {
    printf("Usage: \n");
    printf("[resolution] [samples] [photons_per_light] [depth]\n");
    printf("Defaults: resolution=256 samples=50 photons_per_light=10000 param4=3\n");
}

int main(int argc, char **argv) {
    srand(time(nullptr));

    u32 resolution = 256;
    u32 samples    = 50;
    u32 photons_per_light     = 10000;
    u32 depth     = 3;

    if (argc > 1) {
        if (std::strcmp(argv[1], "-h") == 0 || std::strcmp(argv[1], "--help") == 0) {
            print_usage();
            return 0;
        }
        resolution = std::atoi(argv[1]);
    }
    if (argc > 2) samples = std::atoi(argv[2]);
    if (argc > 3) photons_per_light  = std::atoi(argv[3]);
    if (argc > 4) depth  = std::atoi(argv[4]);

    printf("Chosen paramters: \n");
    printf("resolution: %i\n", resolution);
    printf("samples: %i\n", samples);
    printf("photons_per_light: %i\n", photons_per_light);
    printf("depth: %i\n", depth);

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
    scene.generate_image(resolution, samples, photons_per_light, depth);

    return 0;
}