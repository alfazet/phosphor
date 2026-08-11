#ifndef PHOSPHOR_OPENCL_HPP
#define PHOSPHOR_OPENCL_HPP

#define CL_HPP_TARGET_OPENCL_VERSION 300
#define CL_HPP_ENABLE_EXCEPTIONS

#include <CL/opencl.hpp>
#include <string>

#include "common.hpp"
#include "logger.hpp"

struct ClContext {
    explicit ClContext();

    void select_platform_and_device();

    cl::Program build_program(const std::string &path, const std::string &build_opts) const;

    std::string device_name() const;

    std::string platform_name() const;

    // platform = an OpenCl impl, e.g. Intel, Nvidia, AMD
    cl::Platform platform;
    // device = CPU or GPU
    cl::Device device;
    cl::Context context;
    cl::CommandQueue queue;
};

#endif // PHOSPHOR_OPENCL_HPP
