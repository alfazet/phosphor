#ifndef PHOSPHOR_OPENCL_CTX_HPP
#define PHOSPHOR_OPENCL_CTX_HPP

#define CL_HPP_TARGET_OPENCL_VERSION 300
#define CL_HPP_ENABLE_EXCEPTIONS

#include "typedefs.h"
#include <CL/opencl.hpp>
#include <string>

constexpr const char *KERNEL_COMPILATION_FLAGS = "-I./include";

struct ClContext {
    explicit ClContext();

    void select_platform_and_device();

    cl::Program build_program(const std::string &path, const std::string &build_opts) const;

    cl::Kernel make_kernel(const char *name) const;

    std::string device_name() const;

    std::string platform_name() const;

    // platform = an OpenCl impl, e.g. Intel, Nvidia, AMD
    cl::Platform platform;
    // device = CPU or GPU
    cl::Device device;
    cl::Context context;
    cl::CommandQueue queue;
};

template <typename... Args> void set_kernel_args(cl::Kernel &kernel, Args &&...args) {
    u32 index = 0;
    (kernel.setArg(index++, std::forward<Args>(args)), ...);
}

#endif // PHOSPHOR_OPENCL_CTX_HPP
