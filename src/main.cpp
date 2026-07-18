#include "cmd_args.hpp"
#include "common.hpp"
#include "light.hpp"
#include "logger.hpp"
#include "printers.hpp"
#include "random.hpp"
#include "scene.hpp"
#include "scene_reader.hpp"

#include <iostream>

void phosphor_main(const ArgsList &args) {
    RngState rng;
    pcg_seed(rng, args.seed);

    auto scenes = read_file(args.model.c_str());
    auto scene = scenes[0];

    vec3 white = vec3(50.0f);
    // scene.add_point_light(PointLight(vec3(0.3f, 2.0f, 0.0f), white));
    // scene.add_point_light(PointLight(vec3(-1.0f, 2.0f, 0.0f), white));
    scene.add_point_light(PointLight(vec3(0.0f, 2.0f, 0.5f), white));
    print_spanning_box(scene);
    print_camera(scene.get_camera());
    scene.generate_image(rng, args.resolution, args.samples, args.photons_per_light, args.depth,
                         args.output_path.c_str());
}

void init_logger() {
    auto &l = logger::Logger::instance();

    l.set_level(logger::Level::Debug);

    auto multi = std::make_unique<logger::MultiSink>();
    multi->add(std::make_unique<logger::ConsoleSink>());
    multi->add(std::make_unique<logger::FileSink>("phosphor.log", false));
    l.set_sink(std::move(multi));
}

int main(int argc, char **argv) {
    init_logger();

    ArgParser arg_parser(argc, argv, std::cout);
    try {
        auto args = arg_parser.parse_all();
        LOG_INFO("chosen parameters:");
        arg_parser.print_values(args);

        phosphor_main(args);
    } catch (const HelpRequested &) {
        arg_parser.print_help();
        return 0;
    } catch (const ArgParseError &e) {
        LOG_ERROR("parsing arguments: {}", e.what());
        arg_parser.print_help();
        return 1;
    }

    return 0;
}
