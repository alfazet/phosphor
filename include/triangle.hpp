#ifndef PHOSPHOR_SPHERE_HPP
#define PHOSPHOR_SPHERE_HPP

#include "common.hpp"
#include "glm/gtx/intersect.hpp"
#include "interval.hpp"
#include "logger.hpp"
#include "material.hpp"
#include "memory"
#include "ray.hpp"
#include "texture.hpp"
#include "bounding_box.hpp"

#include <optional>

struct HitRecord {
    vec3 point;
    vec3 normal;
    f32 t;
    vec2 bary;
    bool front_face;

    void set_face_normal(const Ray &r, const vec3 &outward_normal) {
        front_face = glm::dot(r.direction, outward_normal) < 0.0f;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

struct Triangle {
    vec3 v0, v1, v2;
    vec2 uv0, uv1, uv2;
    vec3 n0, n1, n2;
    vec3 t0, t1, t2;
    Material mat;

    Triangle(const vec3 &v0, const vec3 &v1, const vec3 &v2, Material &mat, const vec2 &uv0, const vec2 &uv1,
             const vec2 &uv2, const vec3 &n0, const vec3 &n1, const vec3 &n2, const vec3 &t0, const vec3 &t1,
             const vec3 &t2)
        : v0(v0), v1(v1), v2(v2), mat(mat), uv0(uv0), uv1(uv1), uv2(uv2), n0(n0), n1(n1), n2(n2), t0(t0),
          t1(t1), t2(t2) {}

    bool hit(const Ray &r, interval t, HitRecord &rec, const std::vector<Texture> &textures) const {
        vec2 bary;
        f32 t_found;
        if (!glm::intersectRayTriangle(r.origin, r.direction, v0, v1, v2, bary, t_found))
            return false;
        if (t_found < t.start || t_found > t.end)
            return false;

        vec3 normal_ = get_normal(bary, textures);
        rec.t = t_found;
        rec.bary = bary;
        rec.point = r.at(t_found);
        rec.set_face_normal(r, normal_);
        return true;
    }

    // https://learnopengl.com/Advanced-Lighting/Normal-Mapping
    vec3 get_normal(const vec2 &bary, const std::vector<Texture> &textures) const {
        vec3 N = glm::normalize(compute_bary(bary, n0, n1, n2));
        // return N;

        if (mat.norm_index.has_value()) {
            // LOG_INFO("N before: {}, {}, {}", N.x, N.y, N.z);
            vec3 normal = normal_sample(&textures[*mat.norm_index], compute_bary(bary, uv0, uv1, uv2));
            vec3 T = glm::normalize(compute_bary(bary, t0, t1, t2));
            T = glm::normalize(T - N * glm::dot(N, T));
            vec3 B = glm::cross(N, T);
            mat3 TBN(T, B, N);
            vec3 qqq = glm::normalize(TBN * normal);
            // LOG_INFO("N after: {}, {}, {}", qqq.x, qqq.y, qqq.z);

            return qqq;
        }
        return N;
    }

    vec3 point_at(f32 u, f32 v) const { return v0 + (v1 - v0) * u + (v2 - v0) * v; }
    vec2 uv_at(f32 u, f32 v) const { return uv0 + (uv1 - uv0) * u + (uv2 - uv0) * v; }
    f32 area() const { return 0.5f * glm::length(glm::cross(v1 - v0, v2 - v0)); }

    BoundingBox get_bounding_box() const {
        interval x{INF, -INF};
        interval y{INF, -INF};
        interval z{INF, -INF};

        auto expand = [&](const vec3 &p) {
            x.start = glm::min(x.start, p.x);
            x.end   = glm::max(x.end,   p.x);
            y.start = glm::min(y.start, p.y);
            y.end   = glm::max(y.end,   p.y);
            z.start = glm::min(z.start, p.z);
            z.end   = glm::max(z.end,   p.z);
        };

        expand(v0);
        expand(v1);
        expand(v2);

        return {x, y, z};
    }
};

#endif // PHOSPHOR_SPHERE_HPP
