#include "cmd_args.hpp"
#include "constants.h"
#include "logger.hpp"
#include "opencl_ctx.hpp"
#include "printers.hpp"
#include "random.h"

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

    cl::Program program = ctx.build_program(std::string(PROJECT_DIR) + "/kernels/hello_world.cl", "-I./include");
    cl::Kernel kernel(program, "hello_world");

    RngState rng = pcg_seed(args.seed);

    // naming convention:
    // h_... = in host memory, d_... = in device memory
    usize n = (1 << 20);
    std::vector<f32> h_a(n), h_b(n);
    for (usize i = 0; i < n; i++) {
        h_a[i] = random_float(rng);
        h_b[i] = random_float(rng);
    }

    // CL_MEM_COPY_HOST_PTR: "[...] it indicates that the application wants the OpenCL implementation
    // to allocate memory for the memory object and copy the data from memory referenced by host_ptr.
    // The implementation will copy the memory immediately and host_ptr is available for reuse [...]"
    // - https://registry.khronos.org/OpenCL/specs/unified/html/OpenCL_API.html#CL_MEM_COPY_HOST_PTR

    // so this is effectively a cudaMalloc + cudaMemcpy at once, with additional flags for read/write-only
    cl::Buffer d_a(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, n * sizeof(f32), h_a.data());
    cl::Buffer d_b(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, n * sizeof(f32), h_b.data());
    cl::Buffer d_c(ctx.context, CL_MEM_WRITE_ONLY, n * sizeof(f32));

    kernel.setArg(0, d_a);
    kernel.setArg(1, d_b);
    kernel.setArg(2, d_c);
    kernel.setArg(3, n);

    // CUDA name | OpenCL name | definition
    // ------------------------------------
    // block | workgroup | a groub of threads launched at once, running the same kernel
    // block size | local size | number of threads in one block
    // grid size | global size | total number of launched threads over all blocks
    cl::NDRange global_size(n); // dim3
    cl::NDRange local_size(256);
    ctx.queue.enqueueNDRangeKernel(kernel, cl::NullRange, global_size, local_size);

    std::vector<f32> h_c(n);
    // the second argument specifies if the read is blocking
    ctx.queue.enqueueReadBuffer(d_c, CL_TRUE, 0, n * sizeof(f32), h_c.data());

    for (usize i = 0; i < n; i++) {
        ASSERT(std::abs(h_c[i] - (h_a[i] + h_b[i])) < EPS, "addition failed");
    }
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
