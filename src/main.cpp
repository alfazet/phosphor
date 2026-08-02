#include "cmd_args.hpp"
#include "common.hpp"
#include "logger.hpp"
#include "printers.hpp"
#include "random.hpp"
#include "scene.hpp"
#include "scene_reader.hpp"

#include <iostream>

void init_logger() {
    auto &l = logger::Logger::instance();

    l.set_level(logger::Level::Debug);

    auto multi = std::make_unique<logger::MultiSink>();
    multi->add(std::make_unique<logger::ConsoleSink>());
    multi->add(std::make_unique<logger::FileSink>("phosphor.log", false));
    l.set_sink(std::move(multi));
}

void write_image_metadata(const ArgsList &args) {
    std::ostringstream comment;
    comment << "resolution=" << args.resolution << " samples=" << args.samples
            << " photons_per_light=" << args.photons_per_light << "photon_depth=" << args.photon_depth
            << "ray_depth=" << args.ray_depth << " n_threads=" << args.n_threads << " image_iters=" << args.image_iters;
    std::ostringstream cmd;
    cmd << "exiftool -q -overwrite_original "
        << "-Comment=\"" << comment.str() << "\" "
        << "\"" << args.output_path << "\"";

    u32 ret = std::system(cmd.str().c_str());
    if (ret != 0)
        LOG_ERROR("exiftool failed to write metadata (exit code {})", ret);
}

void phosphor_main(const ArgsList &args) {
    auto scenes = read_file(args.model.c_str());
    usize scene_index = 0;
    LOG_INFO("using scene {}", scene_index);
    if (scenes.size() == 0)
        LOG_FATAL("scene not found");
    auto scene = scenes[scene_index];

    print_spanning_box(scene);
    print_camera(scene.get_camera());

    RngState rng = pcg_seed(args.seed);
    scene.generate_image(std::move(rng), args.resolution, args.samples, args.photons_per_light, args.photon_depth,
                         args.ray_depth, args.output_path.c_str(), args.n_threads, args.image_iters);

    write_image_metadata(args);
}

i32 main(i32 argc, char **argv) {
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
