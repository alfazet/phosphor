#include <algorithm>
#include <vector>

#include "../utils.hpp"
#include "catch2/catch_approx.hpp"
#include "common/light.h"
#include "common/typedefs.h"
#include "host/opencl_ctx.hpp"
#include "host/scene_data.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

struct SceneBounds {
    float4 center{{0.0f, 0.0f, 0.0f, 0.0f}};
    f32 radius = 1.0f;
};

struct Probe {
    cl::Kernel kernel;
    cl::Buffer lights, pref_sum, dummy;
    u32 n_lights = 0;
    u32 n_tris = 0;
    f32 total_lum = 0.0f;

    // null unless the scene has area lights
    cl::Buffer etri_v0, etri_e1, etri_e2, etri_uv;

    float4 scene_center{{0.0f, 0.0f, 0.0f, 0.0f}};
    f32 scene_radius = 1.0f;
};

struct AreaTriangle {
    float4 v0, e1, e2;
    float4 uv;
};

inline AreaTriangle area_tri(float4 a, float4 b, float4 c, float4 uv = float4{{0.0f, 0.0f, 0.0f, 0.0f}}) {
    return AreaTriangle{a, {{b.x - a.x, b.y - a.y, b.z - a.z, 0.0f}}, {{c.x - a.x, c.y - a.y, c.z - a.z, 0.0f}}, uv};
}

inline const cl::Buffer &or_dummy(const cl::Buffer &b, const cl::Buffer &fallback) {
    return b() != nullptr ? b : fallback;
}

struct Samples {
    std::vector<float4> origin, dir, power;
    usize size() const { return origin.size(); }
};

inline Probe make(const ClContext &ctx, std::vector<Light> lights, const std::vector<AreaTriangle> &tris,
                  SceneBounds bounds = {}) {
    Probe p;
    p.n_lights = static_cast<u32>(lights.size());
    p.scene_center = bounds.center;
    p.scene_radius = bounds.radius;

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

    if (p.n_tris > 0) {
        std::vector<float4> v0(p.n_tris), e1(p.n_tris), e2(p.n_tris), uv(p.n_tris);
        for (u32 i = 0; i < p.n_tris; i++) {
            v0[i] = tris[i].v0;
            e1[i] = tris[i].e1;
            e2[i] = tris[i].e2;
            uv[i] = tris[i].uv;
        }
        auto upload = [&](const std::vector<float4> &src) {
            return cl::Buffer(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float4) * src.size(),
                              const_cast<float4 *>(src.data()));
        };
        p.etri_v0 = upload(v0);
        p.etri_e1 = upload(e1);
        p.etri_e2 = upload(e2);
        p.etri_uv = upload(uv);
    }

    return p;
}

inline Probe make(const ClContext &ctx, std::vector<Light> lights) {
    return make(ctx, std::move(lights), {}, SceneBounds{});
}

inline Probe make(const ClContext &ctx, std::vector<Light> lights, SceneBounds bounds) {
    return make(ctx, std::move(lights), {}, bounds);
}

inline Samples run_sample_light(const ClContext &ctx, Probe &p, u32 n, u32 seed) {
    Samples s;
    s.origin.assign(n, float4{});
    s.dir.assign(n, float4{});
    s.power.assign(n, float4{});

    cl::Buffer bo(ctx.context, CL_MEM_WRITE_ONLY, sizeof(float4) * n);
    cl::Buffer bd(ctx.context, CL_MEM_WRITE_ONLY, sizeof(float4) * n);
    cl::Buffer bp(ctx.context, CL_MEM_WRITE_ONLY, sizeof(float4) * n);

    set_kernel_args(p.kernel, p.lights, p.n_lights, p.pref_sum, p.total_lum, p.scene_center, p.scene_radius,
                    or_dummy(p.etri_v0, p.dummy), or_dummy(p.etri_e1, p.dummy), or_dummy(p.etri_e2, p.dummy),
                    or_dummy(p.etri_uv, p.dummy), p.dummy, p.dummy, p.dummy, p.dummy, seed, n, bo, bd, bp);

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

f32 rel_err(const float4 &v, const float4 &ex) {
    return std::max({std::abs(v.x - ex.x) / ex.x, std::abs(v.y - ex.y) / ex.y, std::abs(v.z - ex.z) / ex.z});
}

f32 abs_err(const float4 &v, const float4 &ex) {
    return std::max({std::abs(v.x - ex.x), std::abs(v.y - ex.y), std::abs(v.z - ex.z)});
}

float4 normalize(const float4 a) { return vec3_to_float4(glm::normalize(float4_to_vec3(a))); }

TEST_CASE("sample point light", "[sample_light]") {
    REQUIRE_CL_DEVICE();

    const float4 l_pos{{0.0f, 2.0f, 0.0f, 0.0f}};
    const float4 l_intensity{{0.5f, 0.25f, 0.125f, 0.0f}};
    const float4 ex_power{{4.0f * PI * l_intensity.x, 4.0f * PI * l_intensity.y, 4.0f * PI * l_intensity.z}};

    Light light{};
    light.kind = LIGHT_POINT;
    light.position = l_pos;
    light.power = l_intensity;

    auto probe = make(gpu(), {light});
    REQUIRE(probe.n_lights == 1u);

    SECTION("per-sample power = 4pi * intensity") {
        constexpr u32 n_samples = 4096;
        auto s = run_sample_light(gpu(), probe, n_samples, 2137);
        REQUIRE(s.size() == n_samples);

        for (u32 i = 0; i < n_samples; i++) {
            const f32 len_sq = dot3(s.dir[i], s.dir[i]);
            CHECK(len_sq == Catch::Approx(1.0f).epsilon(1e-4));

            f32 origin_dev = std::max({std::abs(s.origin[i].x - l_pos.x), std::abs(s.origin[i].y - l_pos.y),
                                       std::abs(s.origin[i].z - l_pos.z)});
            CHECK(origin_dev <= 1e-6);

            CHECK(rel_err(s.power[i], ex_power) <= 1e-6);

            if (i > 0)
                CHECK(!same_dir(s.dir[i], s.dir[0]));
        }
    }

    SECTION("mean of per-sample power = total flux 4pi * intensity") {
        const u32 n_samples = GENERATE(1u << 10, 1u << 16);

        auto s = run_sample_light(gpu(), probe, n_samples, 2137);
        REQUIRE(s.size() == n_samples);

        double sum[4]{};
        for (u32 i = 0; i < n_samples; i++) {
            sum[0] += s.power[i].x;
            sum[1] += s.power[i].y;
            sum[2] += s.power[i].z;
            sum[3] += s.power[i].w;
        }

        const f32 want[3] = {ex_power.x, ex_power.y, ex_power.z};
        for (u32 c = 0; c < 3; c++) {
            const f32 mean = sum[c] / n_samples;
            INFO("channel " << c << ": mean=" << mean << " want=" << want[c]);
            CHECK(std::abs(mean - want[c]) <= 1e-6 * want[c]);
        }
        CHECK(sum[3] == 0.0);
    }

    SECTION("same seed reproduces, different seed diverges") {
        const u32 n_samples = 256;

        auto a = run_sample_light(gpu(), probe, n_samples, 2137);
        auto b = run_sample_light(gpu(), probe, n_samples, 2137);
        auto c = run_sample_light(gpu(), probe, n_samples, 42);
        REQUIRE(a.size() == n_samples);
        REQUIRE(b.size() == n_samples);
        REQUIRE(c.size() == n_samples);

        u32 identical = 0, differs = 0;
        for (u32 i = 0; i < n_samples; i++) {
            if (a.dir[i].x == b.dir[i].x && a.dir[i].y == b.dir[i].y && a.dir[i].z == b.dir[i].z)
                identical++;
            if (!same_dir(a.dir[i], c.dir[i]))
                differs++;
        }

        CHECK(identical == n_samples);
        CHECK(differs == n_samples);
    }
}

TEST_CASE("sample spot light", "[sample_light]") {
    REQUIRE_CL_DEVICE();

    const float4 l_pos{{0.0f, 3.0f, 0.0f, 0.0f}};
    const float4 l_int{{0.5f, 0.25f, 0.125f, 0.0f}};

    const float4 axis = GENERATE(float4{{0.0f, -1.0f, 0.0f, 0.0f}}, float4{{0.4f, -1.0f, 0.3f, 0.0f}});
    const f32 half_angle = GENERATE(0.35f, 0.7f);

    const float4 n_axis = normalize(axis);
    const f32 cos_out = std::cos(half_angle);
    const f32 omega = 2.0f * PI * (1.0f - cos_out);
    const float4 flux{{l_int.x * omega, l_int.y * omega, l_int.z * omega, 0.0f}};

    Light light{};
    light.kind = LIGHT_SPOT;
    light.position = l_pos;
    light.power = l_int;
    light.direction = axis;
    light.aux = float4{{half_angle, half_angle, 0.0f, 0.0f}};

    auto probe = make(gpu(), {light});
    REQUIRE(probe.n_lights == 1u);

    constexpr u32 n = 1u << 16;
    auto s = run_sample_light(gpu(), probe, n, 2137);
    REQUIRE(s.size() == n);

    double sum[3]{};

    for (u32 i = 0; i < n; i++) {
        CHECK(dot3(s.dir[i], s.dir[i]) == Catch::Approx(1.0f).epsilon(1e-4));
        CHECK(abs_err(s.origin[i], l_pos) <= 1e-6);

        // must not emit outside its cone
        const f32 c = dot3(s.dir[i], n_axis);
        CHECK(c >= cos_out - 1e-4f);

        if (c >= cos_out + 1e-4f)
            CHECK(rel_err(s.power[i], flux) <= 1e-6);

        sum[0] += s.power[i].x;
        sum[1] += s.power[i].y;
        sum[2] += s.power[i].z;
    }

    for (int ch = 0; ch < 3; ch++) {
        const double got = sum[ch] / n;
        const double want = (ch == 0 ? l_int.x : ch == 1 ? l_int.y : l_int.z) * omega;
        INFO("channel " << ch << ": mean=" << got << " want=" << want);
        CHECK(std::abs(got - want) <= 1e-3 * want);
    }
}

TEST_CASE("sample directional light", "[sample_light]") {
    REQUIRE_CL_DEVICE();

    const float4 l_dir = GENERATE(float4{{0.0f, -1.0f, 0.0f, 0.0f}}, float4{{0.3f, -1.0f, 0.4f, 0.0f}});
    const float4 l_int{{2.0f, 1.0f, 0.5f, 0.0f}};

    Light light{};
    light.kind = LIGHT_DIRECTIONAL;
    light.direction = l_dir;
    light.power = l_int;

    const SceneBounds bounds{{{0.0f, -3.0f, 0.0f, 0.0f}}, 2.5f};
    auto probe = make(gpu(), {light}, bounds);
    REQUIRE(probe.n_lights == 1u);
    REQUIRE(probe.scene_radius == bounds.radius);

    const float4 c = probe.scene_center;
    const f32 r = probe.scene_radius;
    const float4 d = normalize(l_dir);
    const float4 flux{{l_int.x * PI * r * r, l_int.y * PI * r * r, l_int.z * PI * r * r, 0.0f}};

    constexpr u32 n = 1u << 16;
    auto s = run_sample_light(gpu(), probe, n, 2137);
    REQUIRE(s.size() == n);

    double sum_rho2 = 0.0;
    for (u32 i = 0; i < n; i++) {
        // every ray travels the same way
        CHECK(dot3(s.dir[i], d) == Catch::Approx(1.0f).epsilon(1e-6));

        const float4 rel{{s.origin[i].x - c.x, s.origin[i].y - c.y, s.origin[i].z - c.z, 0.0f}};
        const f32 along = dot3(rel, d);
        CHECK(along == Catch::Approx(-2.0f * r).margin(1e-4));

        const float4 lat{{rel.x - d.x * along, rel.y - d.y * along, rel.z - d.z * along, 0.0f}};
        const f32 rho = std::sqrt(dot3(lat, lat));
        CHECK(rho <= r + 1e-4f); // inside the disk of radius r
        sum_rho2 += rho * rho;

        // flux through the disk: intensity * area
        CHECK(rel_err(s.power[i], flux) <= 1e-6);
    }

    const double tol = 5.0 * r * r / std::sqrt(12.0 * n); // Var(rho^2) = r^4/12 for a uniform disk
    INFO("E[rho^2]=" << sum_rho2 / n << " want=" << 0.5 * r * r << " tol=" << tol);
    CHECK(std::abs(sum_rho2 / n - 0.5 * r * r) <= tol);
}
