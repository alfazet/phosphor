#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>

#include "common/bounding_box.h"
#include "common/constants.h"
#include "utils.hpp"

using Catch::Approx;
using namespace ts;

TEST_CASE("BoundingBox layout is stable", "[bbox][layout]") {
    STATIC_REQUIRE(sizeof(BoundingBox) == 32);
    STATIC_REQUIRE(alignof(BoundingBox) % 16 == 0);
    STATIC_REQUIRE(offsetof(BoundingBox, min) == 0);
    STATIC_REQUIRE(offsetof(BoundingBox, max) == 16);
}

TEST_CASE("check_inf rejects only the sentinel extremes", "[bbox]") {
    CHECK(check_inf(INF));
    CHECK(check_inf(-INF));
    CHECK_FALSE(check_inf(0.0f));
    CHECK_FALSE(check_inf(1.0f));
    CHECK_FALSE(check_inf(-1.0f));
}

TEST_CASE("get_bounding_box spans the vertices and expands by BBOX_EPS", "[bbox]") {
    const BoundingBox b = get_bounding_box(make_triangle(f4(1, 2, 3), f4(-1, 5, 0), f4(0, -2, 4)));

    CHECK(b.min.x == Approx(-1.0f - BBOX_EPS));
    CHECK(b.min.y == Approx(-2.0f - BBOX_EPS));
    CHECK(b.min.z == Approx(0.0f - BBOX_EPS));
    CHECK(b.max.x == Approx(1.0f + BBOX_EPS));
    CHECK(b.max.y == Approx(5.0f + BBOX_EPS));
    CHECK(b.max.z == Approx(4.0f + BBOX_EPS));

    const BoundingBox permuted = get_bounding_box(make_triangle(f4(0, -2, 4), f4(1, 2, 3), f4(-1, 5, 0)));
    CHECK(same_box(permuted, b));
}

TEST_CASE("a flat triangle still gets a non-zero width", "[bbox]") {
    const BoundingBox b = get_bounding_box(make_triangle(f4(0, 1, 0), f4(2, 1, 0), f4(4, 1, 0)));

    CHECK(b.max.y - b.min.y == Approx(2.0f * BBOX_EPS).margin(1e-7));
    CHECK(b.max.x - b.min.x == Approx(4.0f + 2.0f * BBOX_EPS).margin(1e-7));
}

TEST_CASE("expand cannot widen an axis at large coordinates", "[bbox]") {
    REQUIRE(BBOX_EPS < 32.0f);

    BoundingBox b = make_bbox(1e9f, 1e9f, 1e9f, 1e9f, 1e9f + 1000.0f, 1e9f + 1000.0f);
    expand(b);

    CHECK(b.min.x == b.max.x);
    CHECK(b.max.y > b.min.y);
}

TEST_CASE("expand_aabb only grows to include the point", "[bbox]") {
    BoundingBox b = make_bbox(0, 0, 0, 1, 1, 1);

    expand_aabb(b, f4(0.5f, 0.5f, 0.5f));
    CHECK(same_box(b, make_bbox(0, 0, 0, 1, 1, 1)));

    expand_aabb(b, f4(-3.0f, 0.5f, 7.0f));
    CHECK(b.min.x == -3.0f);
    CHECK(b.max.z == 7.0f);
    CHECK(b.max.x == 1.0f);
    CHECK(b.min.y == 0.0f);
}

TEST_CASE("get_longest_axis returns 0/1/2 for x/y/z and prefers z on ties", "[bbox]") {
    struct Case {
        f32 ex, ey, ez;
        u32 expect;
    };

    const Case cases[] = {
        {4, 2, 1, 0}, {2, 4, 1, 1}, {2, 1, 4, 2}, {4, 4, 1, 1}, {4, 1, 4, 2}, {1, 4, 4, 2}, {3, 3, 3, 2},
    };

    for (const Case &c : cases) {
        CAPTURE(c.ex, c.ey, c.ez);
        CHECK(get_longest_axis(make_bbox(0, 0, 0, c.ex, c.ey, c.ez)) == c.expect);
    }
}

TEST_CASE("merge is commutative and cumulative", "[bbox]") {
    const BoundingBox a = make_bbox(0, 0, 0, 2, 2, 2);
    const BoundingBox b = make_bbox(-1, 1, 5, 1, 4, 6);
    const BoundingBox ab = merge(a, b);

    CHECK(ab.min.x == -1.0f);
    CHECK(ab.min.y == 0.0f);
    CHECK(ab.min.z == 0.0f);
    CHECK(ab.max.x == 2.0f);
    CHECK(ab.max.y == 4.0f);
    CHECK(ab.max.z == 6.0f);

    CHECK(same_box(merge(b, a), ab));
    CHECK(same_box(merge(a, a), a));
    CHECK(same_box(merge(ab, a), ab));
    CHECK(same_box(merge(ab, b), ab));
}
