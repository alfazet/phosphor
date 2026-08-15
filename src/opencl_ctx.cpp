#include "opencl_ctx.hpp"
#include "logger.hpp"

#include <fstream>

ClContext::ClContext() {
    this->select_platform_and_device();
    this->context = cl::Context(this->device);
    // TODO: take a look at properties again
    this->queue = cl::CommandQueue(this->context, this->device, cl::QueueProperties::None);

    LOG_INFO("OpenCL context set up successfully");
}

void ClContext::select_platform_and_device() {
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    if (platforms.empty()) {
        throw std::runtime_error("no OpenCL platforms found");
    }

    // prefer a GPU
    for (const auto &p : platforms) {
        std::vector<cl::Device> devices;
        p.getDevices(CL_DEVICE_TYPE_GPU, &devices);
        if (!devices.empty()) {
            this->platform = p;
            this->device = devices.front();
            return;
        }
    }

    // Fallback to any available device
    for (const auto &p : platforms) {
        std::vector<cl::Device> devices;
        p.getDevices(CL_DEVICE_TYPE_ALL, &devices);
        if (!devices.empty()) {
            this->platform = p;
            this->device = devices.front();
            return;
        }
    }

    throw std::runtime_error("no OpenCL devices found on any platform");
}

cl::Program ClContext::build_program(const std::string &path, const std::string &build_opts) const {
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("could not open kernel source file `" + path + "`");
    std::ostringstream ss;
    ss << file.rdbuf();

    cl::Program program(this->context, ss.str());
    try {
        program.build(this->device, build_opts.c_str());
    } catch (const cl::Error &) {
        const std::string log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(this->device);
        throw std::runtime_error("kernel build failed (" + log + ")");
    }

    return program;
}

std::string ClContext::device_name() const { return device.getInfo<CL_DEVICE_NAME>(); }

std::string ClContext::platform_name() const { return platform.getInfo<CL_PLATFORM_NAME>(); }
