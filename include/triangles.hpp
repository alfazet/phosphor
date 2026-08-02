#ifndef PHOSPHOR_TRIANGLES_H
#define PHOSPHOR_TRIANGLES_H

#include "common.hpp"
#include "interval.hpp"
#include "ray.hpp"
#include "triangle.hpp"

struct Triangles {
    std::shared_ptr<std::vector<Triangle>> objects;
    u32 start;
    u32 end;

    std::shared_ptr<Triangles> left;
    std::shared_ptr<Triangles> right;
    BoundingBox boundingBox;

    explicit Triangles();
    Triangles(std::shared_ptr<std::vector<Triangle>> triangles, u32 start, u32 end);

    const Triangle &at(u32 i) const;

    void split();

    bool hit(const Ray &r, Interval t, HitRecord &rec, Material &mat_out, vec2 &uv,
             const std::vector<Texture> &textures, const Triangle *&tri_out) const;
};

#endif // PHOSPHOR_TRIANGLES_H
