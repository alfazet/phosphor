#ifndef PHOSPHOR_INTERVAL_H
#define PHOSPHOR_INTERVAL_H

#include "common.hpp"

// https://raytracing.github.io/books/RayTracingTheNextWeek.html#boundingvolumehierarchies
struct Interval {
    f32 start;
    f32 end;

    explicit Interval() {
        start = +INF;
        end = -INF;
    }

    [[nodiscard]] f32 size() const { return end - start; }

    Interval(f32 a, f32 b) {
        start = a;
        end = b;
    }

    Interval(const Interval &a, const Interval &b) {
        start = glm::min(a.start, b.start);
        end = glm::max(a.end, b.end);
    }

    f32 clamp(f32 x) const {
        if (x < start) {
            return start;
        }
        if (x > end) {
            return end;
        }
        return x;
    }

    Interval expand(f32 delta = 0.01f) const { return Interval(start - delta, end + delta); }
};

inline bool overlap(Interval a, Interval b) { return glm::max(a.start, b.start) < glm::min(a.end, b.end); }

#endif // PHOSPHOR_INTERVAL_H
