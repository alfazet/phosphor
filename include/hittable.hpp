#ifndef PHOSPHOR_SPHERE_HPP
#define PHOSPHOR_SPHERE_HPP

#include "common.hpp"
#include "glm/gtx/intersect.hpp"
#include "material.hpp"
#include "ray.hpp"

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
    Triangle(const vec3 &v0, const vec3 &v1, const vec3 &v2, const Material &mat, const vec2 &uv0, const vec2 &uv1,
             const vec2 &uv2, const std::optional<usize> diff_index, const std::optional<usize> emis_index)
        : v0_(v0), v1_(v1), v2_(v2), uv0_(uv0), uv1_(uv1), uv2_(uv2), diff_index_(diff_index), emis_index_(emis_index), mat_(mat) {
        normal_ = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    }

    bool hit(const Ray &r, f32 t_min, f32 t_max, HitRecord &rec) const {
        vec2 bary;
        f32 t;
        if (!glm::intersectRayTriangle(r.origin, r.direction, v0_, v1_, v2_, bary, t))
            return false;
        if (t < t_min || t > t_max)
            return false;

        rec.t = t;
        rec.bary = bary;
        rec.point = r.at(t);
        rec.set_face_normal(r, normal_);
        return true;
    }

    vec3 v0_, v1_, v2_;
    vec2 uv0_, uv1_, uv2_;
    std::optional<usize> diff_index_;
    std::optional<usize> emis_index_;

    const Material &mat() const { return mat_; }
    const vec3 &normal() const { return normal_; }

    vec3 point_at(f32 u, f32 v) const { return v0_ + (v1_ - v0_) * u + (v2_ - v0_) * v; }

    vec2 uv_at(f32 u, f32 v) const { return uv0_ + (uv1_ - uv0_) * u + (uv2_ - uv0_) * v; }

    f32 area() const { return 0.5f * glm::length(glm::cross(v1_ - v0_, v2_ - v0_)); }

  private:
    vec3 normal_;
    Material mat_;
};

class Sphere {
  public:
    Sphere(const vec3 &center, f32 radius, const Material &mat) : center_(center), radius_(radius), mat_(mat) {}

    bool hit(const Ray &r, f32 t_min, f32 t_max, HitRecord &rec) const {
        const vec3 oc = r.origin - center_;
        const f32 a = glm::dot(r.direction, r.direction);
        const f32 half_b = glm::dot(oc, r.direction);
        const f32 c = glm::dot(oc, oc) - radius_ * radius_;

        const f32 discriminant = half_b * half_b - a * c;
        if (discriminant < 0.0f)
            return false;

        const f32 sqrt_d = glm::sqrt(discriminant);

        f32 root = (-half_b - sqrt_d) / a;
        if (root < t_min || root > t_max) {
            root = (-half_b + sqrt_d) / a;
            if (root < t_min || root > t_max)
                return false;
        }

        rec.t = root;
        rec.point = r.at(root);
        const vec3 outward_normal = (rec.point - center_) / radius_;
        rec.set_face_normal(r, outward_normal);

        return true;
    }

    const vec3 &center() const { return center_; }
    f32 radius() const { return radius_; }
    const Material &mat() const { return mat_; }

  private:
    vec3 center_;
    f32 radius_;
    Material mat_;
};

#endif // PHOSPHOR_SPHERE_HPP
