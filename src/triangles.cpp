#include "triangles.hpp"

Triangles::Triangles() {
    triangles_ = {};
}

bool Triangles::hit(const Ray &r, interval t, HitRecord &rec, Material &mat_out, vec2 &uv, const std::vector<Texture> &textures) const {
    HitRecord temp;
    bool hit_anything = false;
    f32 closest = t.end;
    const Triangle *closest_t = nullptr;

    // space for improvement - do not check all objects in scene
    for (const auto &object : triangles_) {
        if (object.hit(r, interval(t.start, closest), temp, textures)) {
            hit_anything = true;
            closest = temp.t;
            rec = temp;
            closest_t = &object;
        }
    }

    if (hit_anything) {
        mat_out = closest_t->mat_;
        uv = compute_bary(rec.bary, closest_t->uv0_, closest_t->uv1_, closest_t->uv2_);
    }

    return hit_anything;
}





