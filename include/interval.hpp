#ifndef PHOSPHOR_INTERVAL_H
#define PHOSPHOR_INTERVAL_H

#include "common.hpp"

// https://raytracing.github.io/books/RayTracingTheNextWeek.html#boundingvolumehierarchies
struct interval {
    f32 start;
    f32 end;

    interval() {
        start = +INF;
        end = -INF;
    }
    interval(f32 a, f32 b) {
        start = a;
        end = b;
    }
    interval(const interval& a, const interval& b) {
        start = glm::min(a.start, b.start);
        end = glm::max(a.end, b.end);
    }

    [[nodiscard]] f32 clamp(f32 x) const {
        if (x < start) { return start; }
        if (x > end) { return end; }
        return x;
    }
    interval expand(f32 delta = 0.01f) const {
        return interval(start-delta, end+delta);
    }
};

inline bool overlap(interval a, interval b) {
    return glm::max(a.start, b.start) < glm::min(a.end, b.end);
}

#endif // PHOSPHOR_INTERVAL_H
