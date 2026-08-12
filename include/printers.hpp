#ifndef PHOSPHOR_PRINTERS_HPP
#define PHOSPHOR_PRINTERS_HPP

#include "opencl_ctx.hpp"

#include <format>

inline void print_opencl_data(const ClContext &ctx) {
    LOG_INFO("using OpenCL platform: {}", ctx.platform_name());
    LOG_INFO("using OpenCL device: {}", ctx.device_name());
}

#endif // PHOSPHOR_PRINTERS_HPP
