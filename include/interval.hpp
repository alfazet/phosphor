#ifndef PHOSPHOR_INTERVAL_H
#define PHOSPHOR_INTERVAL_H

// https://raytracing.github.io/books/RayTracingTheNextWeek.html#boundingvolumehierarchies
struct interval {
    f32 start;
    f32 end;

    [[nodiscard]] f32 clamp(f32 x) const {
        if (x < start) { return start; }
        if (x > end) { return end; }
        return x;
    }
    interval expand(f32 delta = EPS) const {
        return interval(start-delta, end+delta);
    }
};

#endif // PHOSPHOR_INTERVAL_H
