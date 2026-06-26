#ifndef PHOSPHOR_SPHERE_HPP
#define PHOSPHOR_SPHERE_HPP

#include "common.hpp"
#include "glm/gtx/intersect.hpp"
#include "material.hpp"
#include "ray.hpp"

struct HitRecord {
    vec3 point;
    vec3 normal;
    f32 t;
    bool front_face;

    void set_face_normal(const Ray &r, const vec3 &outward_normal) {
        front_face = glm::dot(r.direction, outward_normal) < 0.0f;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class Triangle {
  public:
    Triangle(const vec3& v0, const vec3& v1, const vec3& v2, const Material& mat) : v0_(v0), v1_(v1), v2_(v2), mat_(mat) {
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
        rec.point = r.at(t);
        rec.set_face_normal(r, normal_);
        return true;
    }

    vec3 v0_, v1_, v2_;

    const Material &mat() const { return mat_; }

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
