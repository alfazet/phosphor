#ifndef PHOSPHOR_BOUNDING_BOX_H
#define PHOSPHOR_BOUNDING_BOX_H

#include "common.hpp"
#include "interval.hpp"
#include "logger.hpp"
#include "ray.hpp"

inline Interval sort_values(f32 a, f32 b) {
    if (a < b)
        return Interval(a, b);
    return Interval(b, a);
}

// https://raytracing.github.io/books/RayTracingTheNextWeek.html#boundingvolumehierarchies
struct BoundingBox {
    Interval x, y, z;

    explicit BoundingBox() {}

    BoundingBox(const Interval &x, const Interval &y, const Interval &z)
        : x(x.expand()), y(y.expand()), z(z.expand()) {}

    BoundingBox(const vec3 &a, const vec3 &b) {
        x = (sort_values(a.x, b.x)).expand();
        y = (sort_values(a.y, b.y)).expand();
        z = (sort_values(a.z, b.z)).expand();
    }

    BoundingBox(const BoundingBox &a, const BoundingBox &b) {
        x = Interval(a.x, b.x).expand();
        y = Interval(a.y, b.y).expand();
        z = Interval(a.z, b.z).expand();
    }

    const Interval &index(u32 i) const {
        switch (i) {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        default:
            LOG_FATAL("no such index in bounding box");
        }
    }

    u32 longest() const {
        if (x.size() > y.size()) {
            return x.size() > z.size() ? 0 : 2;
        }
        return y.size() > z.size() ? 1 : 2;
    }

    bool hit(const Ray &r, Interval t) const {
        for (u32 axis = 0; axis < 3; axis++) {
            const Interval &ax = index(axis);
            const f32 adinv = 1.0f / r.direction[axis];

            f32 t0 = (ax.start - r.origin[axis]) * adinv;
            f32 t1 = (ax.end - r.origin[axis]) * adinv;

            if (t0 < t1) {
                if (t0 > t.start)
                    t.start = t0;
                if (t1 < t.end)
                    t.end = t1;
            } else {
                if (t1 > t.start)
                    t.start = t1;
                if (t0 < t.end)
                    t.end = t0;
            }

            if (t.end <= t.start)
                return false;
        }

        return true;
    }
};

#endif // PHOSPHOR_BOUNDING_BOX_H
