#ifndef PHOSPHOR_SPHERE_HPP
#define PHOSPHOR_SPHERE_HPP

#include "common.hpp"
#include "glm/gtx/intersect.hpp"
#include "logger.hpp"
#include "material.hpp"
#include "memory"
#include "ray.hpp"
#include "texture.hpp"

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

class Triangle {
  public:
    Triangle(const vec3 &v0, const vec3 &v1, const vec3 &v2, Material &mat, const vec2 &uv0, const vec2 &uv1,
             const vec2 &uv2, const vec3 &n0, const vec3 &n1, const vec3 &n2)
        : v0_(v0), v1_(v1), v2_(v2), mat_(mat), uv0_(uv0), uv1_(uv1), uv2_(uv2), n0_(n0), n1_(n1), n2_(n2) {}

    bool hit(const Ray &r, f32 t_min, f32 t_max, HitRecord &rec, const std::vector<Texture> &textures) const {
        vec2 bary;
        f32 t;
        if (!glm::intersectRayTriangle(r.origin, r.direction, v0_, v1_, v2_, bary, t))
            return false;
        if (t < t_min || t > t_max)
            return false;

        vec3 normal_ = get_normal(bary, textures);
        rec.t = t;
        rec.bary = bary;
        rec.point = r.at(t);
        rec.set_face_normal(r, normal_);
        return true;
    }

    // https://learnopengl.com/Advanced-Lighting/Normal-Mapping
    vec3 compute_tangent() const {
        vec3 edge1 = v1_ - v0_;
        vec3 edge2 = v2_ - v0_;
        vec2 duv1 = uv1_ - uv0_;
        vec2 duv2 = uv2_ - uv0_;

        f32 det = duv1.x * duv2.y - duv2.x * duv1.y;
        if (glm::abs(det) < EPS)
            return vec3(1, 0, 0);
        f32 f = 1.0f / det;

        return glm::normalize(f * (duv2.y * edge1 - duv1.y * edge2));
    }

    vec3 get_normal(const vec2 &bary, const std::vector<Texture> &textures) const {
        if (mat_.norm_index.has_value()) {
            vec3 normal = normal_sample(&textures[*mat_.norm_index], compute_bary(bary, uv0_, uv1_, uv2_));
            vec3 N = glm::normalize(compute_bary(bary, n0_, n1_, n2_));
            vec3 T = compute_tangent();
            T = glm::normalize(T - N * glm::dot(N, T));
            vec3 B = glm::cross(N, T);
            mat3 TBN(T, B, N);

            return glm::normalize(TBN * normal);
        }
        return glm::normalize(compute_bary(bary, n0_, n1_, n2_));
    }

    vec3 v0_, v1_, v2_;
    vec2 uv0_, uv1_, uv2_;
    vec3 n0_, n1_, n2_;
    Material mat_;

    vec3 point_at(f32 u, f32 v) const { return v0_ + (v1_ - v0_) * u + (v2_ - v0_) * v; }
    vec2 uv_at(f32 u, f32 v) const { return uv0_ + (uv1_ - uv0_) * u + (uv2_ - uv0_) * v; }
    f32 area() const { return 0.5f * glm::length(glm::cross(v1_ - v0_, v2_ - v0_)); }
};

#endif // PHOSPHOR_SPHERE_HPP
