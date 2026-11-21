#ifndef PHOSPHOR_DEVICE_TEST_UTILS
#define PHOSPHOR_DEVICE_TEST_UTILS

#include "host/opencl_ctx.hpp"
#include <string>

inline std::string kernel(const char *name) {
#ifdef PHOSPHOR_TEST_KERNEL_DIR
    return std::string(PHOSPHOR_TEST_KERNEL_DIR) + "/" + name;
#else
    return std::string("tests/device/") + name;
#endif
}

inline std::string kernel_build_opts() {
#ifdef PHOSPHOR_KERNEL_DIR
    return std::string("-I") + PHOSPHOR_KERNEL_DIR + " -cl-std=CL1.2";
#else
    return std::string("-cl-std=CL1.2");
#endif
}

static ClContext &gpu() {
    static ClContext ctx;
    return ctx;
}

#define REQUIRE_CL_DEVICE()                                                                                            \
    do {                                                                                                               \
        if (gpu().device_name().empty())                                                                               \
            SKIP("no OpenCL device");                                                                                  \
    } while (false)

#endif // PHOSPHOR_DEVICE_TEST_UTILS
