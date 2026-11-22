#include <algorithm>
#include <vector>

#include "../utils.hpp"
#include "catch2/catch_approx.hpp"
#include "common/light.h"
#include "common/typedefs.h"
#include "host/logger.hpp"
#include "host/opencl_ctx.hpp"
#include "host/scene_data.hpp"

#include <catch2/catch_test_macros.hpp>

struct Probe {
    cl::Kernel kernel;
    cl::Buffer lights, pref_sum, dummy;
    u32 n_lights = 0;
    f32 total_lum = 0.0f;
};

struct Samples {
    std::vector<float4> origin, dir, power;
    usize size() const { return origin.size(); }
};

inline Probe make(const ClContext &ctx, std::vector<Light> lights) {
    Probe p;
    p.n_lights = static_cast<u32>(lights.size());

    SceneData scene;
    scene.lights = std::move(lights);
    scene.build_lum_pref_sum();
    p.total_lum = scene.lum_pref_sum.empty() ? 0.0f : scene.lum_pref_sum.back();

    auto program = ctx.build_program(kernel("light/light_probe.cl"), kernel_build_opts());
    p.kernel = cl::Kernel(program, "probe_sample_light");

    const usize n_l = std::max<usize>(p.n_lights, 1);
    Light pad_light{};
    auto *light_src = scene.lights.empty() ? &pad_light : scene.lights.data();
    p.lights = cl::Buffer(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Light) * n_l, light_src);

    const usize n_p = std::max<usize>(scene.lum_pref_sum.size(), 1);
    f32 pad_f = 0.0f;
    auto *pref_src = scene.lum_pref_sum.empty() ? &pad_f : scene.lum_pref_sum.data();
    p.pref_sum = cl::Buffer(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(f32) * n_p, pref_src);

    p.dummy = cl::Buffer(ctx.context, CL_MEM_READ_ONLY, 1024);
    return p;
}

inline Samples run_sample_light(const ClContext &ctx, Probe &p, u32 n, u32 seed) {
    Samples s;
    s.origin.assign(n, float4{});
    s.dir.assign(n, float4{});
    s.power.assign(n, float4{});

    cl::Buffer bo(ctx.context, CL_MEM_WRITE_ONLY, sizeof(float4) * n);
    cl::Buffer bd(ctx.context, CL_MEM_WRITE_ONLY, sizeof(float4) * n);
    cl::Buffer bp(ctx.context, CL_MEM_WRITE_ONLY, sizeof(float4) * n);

    set_kernel_args(p.kernel, p.lights, p.n_lights, p.pref_sum, p.total_lum, float4{0.0f, 0.0f, 0.0f, 0.0f}, 1.0f,
                    p.dummy, p.dummy, p.dummy, p.dummy, p.dummy, p.dummy, p.dummy, p.dummy, seed, n, bo, bd, bp);

    const u32 wg = static_cast<u32>(std::min<usize>(64, ctx.max_work_group_size()));
    const u32 global = ((n + wg - 1) / wg) * wg;

    ctx.queue.enqueueNDRangeKernel(p.kernel, cl::NullRange, cl::NDRange(global), cl::NDRange(wg));
    ctx.queue.enqueueReadBuffer(bo, CL_TRUE, 0, sizeof(float4) * n, s.origin.data());
    ctx.queue.enqueueReadBuffer(bd, CL_TRUE, 0, sizeof(float4) * n, s.dir.data());
    ctx.queue.enqueueReadBuffer(bp, CL_TRUE, 0, sizeof(float4) * n, s.power.data());
    return s;
}

f32 dot3(const float4 &a, const float4 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

bool same_dir(const float4 &a, const float4 &b) { return a.x == b.x && a.y == b.y && a.z == b.z; }

TEST_CASE("point light flux = 4*pi*intensity", "[sample_light]") {
    REQUIRE_CL_DEVICE();

    const float4 l_pos{0.0f, 2.0f, 0.0f, 0.0f};
    const float4 l_intensity{0.5f, 0.25f, 0.125f, 0.0f};

    Light light{};
    light.kind = LIGHT_POINT;
    light.position = l_pos;
    light.power = l_intensity;

    auto probe = make(gpu(), {light});
    REQUIRE(probe.n_lights == 1u);

    constexpr u32 n_samples = 4096;
    auto s = run_sample_light(gpu(), probe, n_samples, 2137);
    REQUIRE(s.size() == n_samples);

    for (u32 i = 0; i < n_samples; i++) {
        const f32 len_sq = dot3(s.dir[i], s.dir[i]);
        CHECK(len_sq == Catch::Approx(1.0f).epsilon(1e-4));

        f32 origin_dev = std::max(
            {std::abs(s.origin[i].x - l_pos.x), std::abs(s.origin[i].y - l_pos.y), std::abs(s.origin[i].z - l_pos.z)});
        CHECK(origin_dev <= 1e-6);

        const f32 ex = 4.0f * PI * l_intensity.x;
        const f32 ey = 4.0f * PI * l_intensity.y;
        const f32 ez = 4.0f * PI * l_intensity.z;
        const f32 err = std::max(
            {std::abs(s.power[i].x - ex) / ex, std::abs(s.power[i].y - ey) / ey, std::abs(s.power[i].z - ez) / ez});
        CHECK(err <= 1e-6f);

        if (i > 0)
            CHECK(!same_dir(s.dir[i], s.dir[0]));
    }
}
