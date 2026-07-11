#include "camera.hpp"
#include "cmd_args.hpp"
#include "common.hpp"
#include "hittable.hpp"
#include "pointlight.hpp"
#include "printers.hpp"
#include "scene.hpp"
#include "scenereader.hpp"

#include <cstring>
#include <ctime>
#include <iostream>

int main(int argc, char **argv) {
    ArgParser arg_parser(argc - 1, argv + 1, std::cout);
    auto args = arg_parser.parse_all();

    srand(args.seed);

    // printf("Chosen paramters: \n");
    // printf("resolution: %i\n", resolution);
    // printf("samples: %i\n", samples);
    // printf("photons_per_light: %i\n", photons_per_light);
    // printf("depth: %i\n", depth);

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
    scene.generate_image(args.resolution, args.samples, args.photons_per_light, args.depth);

    return 0;
}
