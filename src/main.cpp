#include "cmd_args.hpp"
#include "constants.h"
#include "logger.hpp"
#include "opencl_ctx.hpp"
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

    cl::Program program = ctx.build_program(std::string(PROJECT_DIR) + "/kernels/ray_test.cl", "-I./include");
    cl::Kernel kernel(program, "ray_test");

    RngState rng = pcg_seed(args.seed);

    usize n = (1 << 20);
    std::vector<float4> h_origin(n), h_dir(n);
    for (usize i = 0; i < n; i++) {
        f32 x = random_float(&rng), y = random_float(&rng), z = random_float(&rng);
        float4 origin = {{x, y, z, 0.0f}};
        x = random_float(&rng), y = random_float(&rng), z = random_float(&rng);
        float4 dir = {{x, y, z, 0.0f}};
    }

    cl::Buffer d_origin(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, n * sizeof(float4), h_origin.data());
    cl::Buffer d_dir(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, n * sizeof(float4), h_dir.data());
    cl::Buffer d_res(ctx.context, CL_MEM_WRITE_ONLY, n * sizeof(float4));

    kernel.setArg(0, d_origin);
    kernel.setArg(1, d_dir);
    kernel.setArg(2, d_res);
    kernel.setArg(3, n);
    kernel.setArg(4, rng);

    cl::NDRange global_size(n);
    cl::NDRange local_size(256);
    ctx.queue.enqueueNDRangeKernel(kernel, cl::NullRange, global_size, local_size);

    std::vector<float4> h_res(n);
    ctx.queue.enqueueReadBuffer(d_res, CL_TRUE, 0, n * sizeof(float4), h_res.data());

    LOG_INFO("finished succesfully");
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
