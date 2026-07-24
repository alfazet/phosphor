#ifndef PHOSPHOR_TRIANGLES_H
#define PHOSPHOR_TRIANGLES_H
#include "common.hpp"
#include "interval.hpp"
#include "ray.hpp"
#include "triangle.hpp"

struct Triangles {
    Triangles();
    Triangles(std::shared_ptr<std::vector<Triangle>> triangles, u32 start, u32 end);
    void split();

    std::shared_ptr<std::vector<Triangle>> objects;
    u32 start;
    u32 end;

    std::shared_ptr<Triangles> left;
    std::shared_ptr<Triangles> right;
    BoundingBox boundingBox;

    bool hit(const Ray &r, interval t, HitRecord &rec, Material &mat_out, vec2 &uv, const std::vector<Texture> &textures) const;
};

#endif // PHOSPHOR_TRIANGLES_H
