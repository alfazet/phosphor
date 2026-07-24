#ifndef PHOSPHOR_TEXTURE_HPP
#define PHOSPHOR_TEXTURE_HPP

#include "common.hpp"
#include "glm/gtx/raw_data.hpp"
#include "stb_image.h"

#include <optional>

struct UVTransform {
    vec2 offset{0.0f, 0.0f};
    f32 rotation = 0.0f;
    vec2 scale{1.0f, 1.0f};
};

struct Texture {
    i32 width;
    i32 height;
    i32 channels;
    std::string name;
    std::vector<u8> data;

    std::optional<UVTransform> uv_transform;
};

enum channels { CHANNEL_R, CHANNEL_G, CHANNEL_B };

inline vec2 apply_uv_transform(vec2 uv, const UVTransform &t) {
    vec2 scaled = uv * t.scale;
    f32 c = glm::cos(t.rotation);
    f32 s = glm::sin(t.rotation);
    vec2 rotated(c * scaled.x - s * scaled.y, s * scaled.x + c * scaled.y);
    return rotated + t.offset;
}

inline vec2 transformed_uv(const Texture *t, vec2 uv) {
    return t->uv_transform.has_value() ? apply_uv_transform(uv, *t->uv_transform) : uv;
}

inline vec3 naive_sample(const Texture *t, vec2 uv) {
    uv.x = uv.x - floor(uv.x);
    uv.y = uv.y - floor(uv.y);
    uv.y = 1.0 - uv.y;
    i32 x = glm::min(static_cast<i32>(uv.x * t->width), t->width - 1);
    i32 y = glm::min(static_cast<i32>(uv.y * t->height), t->height - 1);

    i32 idx = (y * t->width + x) * t->channels;
    return vec3(t->data[idx], t->data[idx + 1], t->data[idx + 2]) / 255.0f;
}

// https://en.wikipedia.org/wiki/Bilinear_interpolation
inline vec3 sample(const Texture *t, vec2 uv) {
    uv = transformed_uv(t, uv);

    uv.x = uv.x - floor(uv.x);
    uv.y = uv.y - floor(uv.y);
    uv.y = 1.0 - uv.y;

    f32 x = glm::min(uv.x * t->width, static_cast<float>(t->width - 1));
    f32 y = glm::min(uv.y * t->height, static_cast<float>(t->height - 1));
    i32 x1 = glm::min(static_cast<i32>(uv.x * t->width), t->width - 1);
    i32 x2 = glm::min(static_cast<i32>(std::ceil(uv.x * t->width)), t->width - 1);
    i32 y1 = glm::min(static_cast<i32>(uv.y * t->height), t->height - 1);
    i32 y2 = glm::min(static_cast<i32>(std::ceil(uv.y * t->height)), t->height - 1);

    i32 idx11 = (y1 * t->width + x1) * t->channels;
    i32 idx12 = (y2 * t->width + x1) * t->channels;
    i32 idx21 = (y1 * t->width + x2) * t->channels;
    i32 idx22 = (y2 * t->width + x2) * t->channels;

    vec3 Q11 = vec3(t->data[idx11], t->data[idx11 + 1], t->data[idx11 + 2]) / 255.0f;
    vec3 Q12 = vec3(t->data[idx12], t->data[idx12 + 1], t->data[idx12 + 2]) / 255.0f;
    vec3 Q21 = vec3(t->data[idx21], t->data[idx21 + 1], t->data[idx21 + 2]) / 255.0f;
    vec3 Q22 = vec3(t->data[idx22], t->data[idx22 + 1], t->data[idx22 + 2]) / 255.0f;

    f32 w11, w12, w21, w22;
    if (x2 == x1 && y2 == y1)
        return naive_sample(t, uv);
    else if (x2 == x1) {
        w11 = (y2 - y) / (y2 - y1);
        w12 = (y - y1) / (y2 - y1);
        w21 = (y2 - y) / (y2 - y1);
        w22 = (y - y1) / (y2 - y1);
    } else if (y2 == y1) {
        w11 = (x2 - x) / (x2 - x1);
        w12 = (x2 - x) / (x2 - x1);
        w21 = (x - x1) / (x2 - x1);
        w22 = (x - x1) / (x2 - x1);
    } else {
        f32 denom = (x2 - x1) * (y2 - y1);
        w11 = (x2 - x) * (y2 - y) / denom;
        w12 = (x2 - x) * (y - y1) / denom;
        w21 = (x - x1) * (y2 - y) / denom;
        w22 = (x - x1) * (y - y1) / denom;
    }

    return w11 * Q11 + w12 * Q12 + w21 * Q21 + w22 * Q22;
}

inline vec3 normal_sample(const Texture *t, vec2 uv) {
    vec3 raw = sample(t, uv);
    return glm::normalize(raw * 2.0f - 1.0f);
}

inline f32 sample(const Texture *t, vec2 uv, channels ch) {
    auto sample_all = sample(t, uv);
    return sample_all[ch];
}

#endif // PHOSPHOR_TEXTURE_HPP
