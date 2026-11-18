#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <cstddef>
#include <set>

#include "common/photon_hash.h"

using Catch::Approx;

namespace {

float4 f4(f32 x, f32 y, f32 z, f32 w = 0.0f) {
    float4 v{};
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;
    return v;
}

BoundingBox make_bbox(f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1) {
    BoundingBox b{};
    b.min.x = x0;
    b.min.y = y0;
    b.min.z = z0;
    b.max.x = x1;
    b.max.y = y1;
    b.max.z = z1;
    return b;
}

PhotonHashData make_hash(f32 ox, f32 oy, f32 oz, f32 cx, f32 cy, f32 cz, u32 res) {
    PhotonHashData d{};
    d.origin = f4(ox, oy, oz);
    d.cell_sizes = f4(cx, cy, cz);
    d.grid_res = res;
    return d;
}

float4 cell_center(const PhotonHashData &d, u32 x, u32 y, u32 z) {
    return f4(d.origin.x + ((f32)x + 0.5f) * d.cell_sizes.x, d.origin.y + ((f32)y + 0.5f) * d.cell_sizes.y,
              d.origin.z + ((f32)z + 0.5f) * d.cell_sizes.z);
}

} // namespace

TEST_CASE("PhotonHashData layout is stable", "[photon_hash][layout]") {
    STATIC_REQUIRE(sizeof(PhotonHashData) == 48);
    STATIC_REQUIRE(alignof(PhotonHashData) % 16 == 0);
    STATIC_REQUIRE(offsetof(PhotonHashData, origin) == 0);
    STATIC_REQUIRE(offsetof(PhotonHashData, cell_sizes) == 16);
    STATIC_REQUIRE(offsetof(PhotonHashData, grid_res) == 32);
}

TEST_CASE("hash packing is pinned", "[photon_hash]") {
    const PhotonHashData d = make_hash(0, 0, 0, 1, 1, 1, 4);

    CHECK(get_hash(f4(0.5f, 0.5f, 0.5f), d) == 1u);  // (0,0,0)
    CHECK(get_hash(f4(0.5f, 0.5f, 1.5f), d) == 2u);  // (0,0,1), z fastest
    CHECK(get_hash(f4(0.5f, 1.5f, 0.5f), d) == 5u);  // (0,1,0), y stride 4
    CHECK(get_hash(f4(1.5f, 0.5f, 0.5f), d) == 17u); // (1,0,0), x stride 16
    CHECK(get_hash(f4(3.5f, 3.5f, 3.5f), d) == 64u); // (3,3,3) = grid_res^3
}

TEST_CASE("origin and per-axis cell sizes are applied", "[photon_hash]") {
    const PhotonHashData offset = make_hash(-10, -20, -30, 2, 2, 2, 4);

    CHECK(get_hash(f4(-9.5f, -19.5f, -29.5f), offset) == 1u);  // (0,0,0)
    CHECK(get_hash(f4(-7.5f, -19.5f, -29.5f), offset) == 17u); // (1,0,0)
    CHECK(get_hash(f4(-9.5f, -17.5f, -29.5f), offset) == 5u);  // (0,1,0)
    CHECK(get_hash(f4(-9.5f, -19.5f, -27.5f), offset) == 2u);  // (0,0,1)

    const PhotonHashData stretched = make_hash(0, 0, 0, 1, 2, 4, 8);

    CHECK(get_hash(f4(1.5f, 0.5f, 0.5f), stretched) == 65u);    // (1,0,0)
    CHECK(get_hash(f4(0.5f, 3.0f, 0.5f), stretched) == 9u);     // (0,1,0)
    CHECK(get_hash(f4(0.5f, 0.5f, 6.0f), stretched) == 2u);     // (0,0,1)
    CHECK(get_hash(f4(4.5f, 10.0f, 14.0f), stretched) == 300u); // (4,5,3)
}

TEST_CASE("positions outside the grid hash to 0", "[photon_hash]") {
    const PhotonHashData d = make_hash(0, 0, 0, 1, 1, 1, 4);

    CHECK(get_hash(f4(-0.001f, 0.5f, 0.5f), d) == 0u);
    CHECK(get_hash(f4(0.5f, -0.001f, 0.5f), d) == 0u);
    CHECK(get_hash(f4(0.5f, 0.5f, -0.001f), d) == 0u);
    CHECK(get_hash(f4(4.0f, 0.5f, 0.5f), d) == 0u);
    CHECK(get_hash(f4(100.0f, 0.5f, 0.5f), d) == 0u);
    CHECK(get_hash(f4(-0.0f, -0.0f, -0.0f), d) == 1u);
}

TEST_CASE("cells map one-to-one onto [1, grid_res^3]", "[photon_hash]") {
    const auto res = GENERATE(1u, 2u, 4u, 16u);
    CAPTURE(res);

    const PhotonHashData d = make_hash(0, 0, 0, 1, 1, 1, res);

    std::set<u32> seen;
    for (u32 x = 0; x < res; ++x)
        for (u32 y = 0; y < res; ++y)
            for (u32 z = 0; z < res; ++z)
                seen.insert(get_hash(cell_center(d, x, y, z), d));

    CHECK(seen.size() == (size_t)res * res * res);
    CHECK(seen.count(0) == 0);
    CHECK(*seen.begin() == 1u);
    CHECK(*seen.rbegin() == (u32)res * res * res);
}

TEST_CASE("27 neighbouring positions hash to 27 distinct cells", "[photon_hash]") {
    const PhotonHashData unit = make_hash(0, 0, 0, 1, 1, 1, 8);
    const PhotonHashData scaled = make_hash(-4, -4, -4, 0.25f, 0.5f, 1.0f, 64);

    for (const PhotonHashData &d : {unit, scaled}) {
        const u32 mid = d.grid_res / 2;
        const float4 p = cell_center(d, mid, mid, mid);

        std::set<u32> seen;
        for (i32 dz = -1; dz <= 1; ++dz)
            for (i32 dy = -1; dy <= 1; ++dy)
                for (i32 dx = -1; dx <= 1; ++dx) {
                    float4 q = p;
                    q.x += (f32)dx * d.cell_sizes.x;
                    q.y += (f32)dy * d.cell_sizes.y;
                    q.z += (f32)dz * d.cell_sizes.z;

                    const u32 h = get_hash(q, d);
                    CHECK(h != 0u);
                    seen.insert(h);
                }

        CHECK(seen.size() == 27);
    }
}

TEST_CASE("build_photon_hash_info derives the grid from the bbox", "[photon_hash]") {
    const BoundingBox bbox = make_bbox(-1, -2, -3, 3, 2, 5);
    const PhotonHashData d = build_photon_hash_info(bbox, 4);

    CHECK(d.grid_res == 4u);
    CHECK(d.origin.x == Approx(-1.0f));
    CHECK(d.origin.y == Approx(-2.0f));
    CHECK(d.origin.z == Approx(-3.0f));
    CHECK(d.cell_sizes.x == Approx(1.0f));
    CHECK(d.cell_sizes.y == Approx(1.0f));
    CHECK(d.cell_sizes.z == Approx(2.0f));
}

TEST_CASE("degenerate axes clamp to MIN_CELL_SIZE", "[photon_hash]") {
    const PhotonHashData slab = build_photon_hash_info(make_bbox(0, 0, 0, 1, 0, 1), 16);
    CHECK(slab.cell_sizes.y == Approx(MIN_CELL_SIZE));
    CHECK(slab.cell_sizes.x > MIN_CELL_SIZE);

    const PhotonHashData point = build_photon_hash_info(make_bbox(5, 5, 5, 5, 5, 5), 16);
    CHECK(point.cell_sizes.x == Approx(MIN_CELL_SIZE));
    CHECK(point.cell_sizes.y == Approx(MIN_CELL_SIZE));
    CHECK(point.cell_sizes.z == Approx(MIN_CELL_SIZE));
    CHECK(get_hash(f4(5, 5, 5), point) == 1u);
}
