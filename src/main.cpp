#include "cmd_args.hpp"
#include "constants.h"
#include "light.h"
#include "logger.hpp"
#include "opencl_ctx.hpp"
#include "photon.h"
#include "printers.hpp"
#include "random.h"
#include "ray.h"

#include <iostream>

void init_logger() {
    auto &l = logger::Logger::instance();

    l.set_level(logger::Level::Debug);

    auto multi = std::make_unique<logger::MultiSink>();
    multi->add(std::make_unique<logger::ConsoleSink>());
    multi->add(std::make_unique<logger::FileSink>("phosphor.log", false));
    l.set_sink(std::move(multi));
}

void phosphor_main(const ArgsList &args) {
    ClContext ctx;
    print_opencl_data(ctx);

    cl::Program program = ctx.build_program(std::string(PROJECT_DIR) + "/kernels/trace_photons.cl", "-I./include");
    cl::Kernel kernel(program, "trace_photons");

    RngState rng = pcg_seed(args.seed);

    usize photons_count = 1 << 20;
    std::vector<Photon> h_photons(photons_count);
    usize h_count = 0;

    std::vector<Light> lights = {Light{
        float4{0.0f, 5.0f, 0.0f, 0.0f},  // position
        float4{1.0f, 1.0f, 1.0f, 0.0f},  // power
        float4{0.0f, -1.0f, 0.0f, 0.0f}, // direction
        float4{1.0f, 0.0f, 0.0f, 0.0f},  // tangent
        float4{0.0f, 0.0f, 1.0f, 0.0f},  // bitangent
        float4{0.0f, 0.0f, 0.0f, 0.0f},  // origin
        float4{0.0f, 0.0f, 0.0f, 0.0f},  // aux
        0,                               // kind
        {}                               // padding
    }};

    cl::Buffer d_photons(ctx.context, CL_MEM_READ_WRITE, photons_count * sizeof(Photon));
    cl::Buffer d_photon_count(ctx.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(usize), &h_count);
    cl::Buffer d_lights(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Light) * lights.size(),
                        lights.data());

    kernel.setArg(0, d_photons);
    kernel.setArg(1, d_photon_count);
    kernel.setArg(2, d_lights);
    kernel.setArg(3, lights.size());
    kernel.setArg(4, photons_count);
    kernel.setArg(5, rng);

    cl::NDRange global_size(photons_count);
    cl::NDRange local_size(256);
    ctx.queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(photons_count), cl::NDRange(256));
    ctx.queue.finish();

    ctx.queue.enqueueReadBuffer(d_photon_count, CL_TRUE, 0, sizeof(usize), &h_count);
    LOG_INFO("stored {} photons", h_count);
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
