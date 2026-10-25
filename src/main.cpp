#include "host/cmd_args.hpp"
#include "host/logger.hpp"
#include "host/opencl_ctx.hpp"
#include "host/parsers/scene.hpp"
#include "host/renderer.hpp"

#include <chrono>
#include <filesystem>
#include <format>
#include <iostream>

i32 main(i32 argc, char **argv) {
    ArgParser arg_parser(argc, argv, std::cout);
    try {
        auto args = arg_parser.parse_all();
        if (args.help) {
            arg_parser.print_help();
            return 1;
        }
        init_logger(args.log_filename.c_str());
        LOG_INFO("chosen parameters:");
        arg_parser.print_values(args);

        ClContext ctx{};
        LOG_INFO("OpenCL platform/device: {}/{}", ctx.platform_name(), ctx.device_name());
        LOG_INFO("max. allocation size: {} bytes", ctx.max_alloc_size());
        LOG_INFO("max. workgroup size: {} threads", ctx.max_work_group_size());

        std::filesystem::path output_dir(args.output_dir);
        if (!args.was_provided("-o")) {
            auto now = std::chrono::system_clock::now();
            std::string timestamp = std::format("{:%Y_%m_%d-%H_%M_%S}", std::chrono::floor<std::chrono::seconds>(now));
            output_dir = std::filesystem::path(args.output_dir + "_" + timestamp);
        }
        std::filesystem::create_directories(output_dir);
        std::string metadata = arg_parser.build_image_metadata(args);

        Renderer renderer(ctx, args, output_dir, metadata);
        SceneData scene = read_gltf_scene(args.model.c_str());
        if (scene.is_empty()) {
            LOG_ERROR("empty scene, nothing to render");
            return 1;
        }
        if (!scene.has_camera()) {
            LOG_ERROR("scene without cameras, nothing to render");
            return 1;
        }
        renderer.set_scene(scene);
        renderer.run();
    } catch (const ArgParseError &e) {
        LOG_ERROR("parsing arguments: {}", e.what());
        arg_parser.print_help();
        return 1;
    } catch (const cl::Error &e) {
        // look up the codes here: https://gist.github.com/bmount/4a7144ce801e5569a0b6
        LOG_ERROR("OpenCL error (code {}): {}", e.err(), e.what());
        return 1;
    }

    return 0;
}
