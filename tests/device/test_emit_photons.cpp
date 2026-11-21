#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "../utils.hpp"
#include "host/opencl_ctx.hpp"

namespace {

static ClContext &gpu() {
    static ClContext ctx;
    return ctx;
}

#define REQUIRE_CL_DEVICE()                                                                                            \
    do {                                                                                                               \
        if (gpu().device_name().empty())                                                                               \
            SKIP("no OpenCL device");                                                                                  \
    } while (false)

} // namespace

TEST_CASE("TODO", "") {
    REQUIRE_CL_DEVICE();
}
