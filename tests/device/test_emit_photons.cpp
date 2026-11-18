#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <tuple>
#include <vector>

#include "common/material.h"
#include "common/triangle.h"
#include "device/encoding.h"
#include "device/hit.h"
#include "device/light_sampling.h"
#include "host/opencl_ctx.hpp"
#include "host/scene_data.hpp"
#include "../utils.hpp"

namespace {

constexpr const char *KERNEL_FILE = "photon_emit";
constexpr const char *KERNEL_NAME = "emit_photons";

static ClContext &gpu() {
    static ClContext *ctx = nullptr;
    static bool tried = false;
    static std::string error;

    if (!tried) {
        tried = true;
        try {
            ctx = new ClContext();
        } catch (const std::exception &e) {
            error = e.what();
        }
    }
    if (!ctx)
        SKIP("OpenCL unavailable: " + error);
    return *ctx;
}

static cl::Kernel &kernel() {
    static cl::Kernel k = gpu().make_kernel(KERNEL_FILE);
    return k;
}

} // namespace

TEST_CASE("", "[device]") {
    cl::Kernel k = kernel();
}

