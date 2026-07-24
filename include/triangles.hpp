#ifndef PHOSPHOR_TRIANGLES_H
#define PHOSPHOR_TRIANGLES_H
#include "common.hpp"
#include "interval.hpp"
#include "ray.hpp"
#include "triangle.hpp"

struct Triangles {
    Triangles();
    std::vector<Triangle> triangles_;

    bool hit(const Ray &r, interval t, HitRecord &rec, Material &mat_out, vec2 &uv, const std::vector<Texture> &textures) const;
};

#endif // PHOSPHOR_TRIANGLES_H
