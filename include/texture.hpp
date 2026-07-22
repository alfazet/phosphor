#ifndef PHOSPHOR_TEXTURE_HPP
#define PHOSPHOR_TEXTURE_HPP

#include "common.hpp"
#include "glm/gtx/raw_data.hpp"
#include "stb_image.h"

#include <filesystem>

struct Texture {
    i32 width;
    i32 height;
    i32 channels;
    std::string name;
    std::vector<u8> data;
};


enum channels { CHANNEL_R, CHANNEL_G, CHANNEL_B };

inline vec3 naive_sample(const Texture *t, vec2 uv) {
    uv.x = uv.x - floor(uv.x);
    uv.y = uv.y - floor(uv.y);
    uv.y = 1.0 - uv.y;
    i32 x = std::min((i32)(uv.x * t->width), t->width - 1);
    i32 y = std::min((i32)(uv.y * t->height), t->height - 1);

    i32 idx = (y * t->width + x) * t->channels;
    return vec3(t->data[idx], t->data[idx + 1], t->data[idx + 2]) / 255.0f;
}

// TODO: different sampling here
// https://en.wikipedia.org/wiki/Bilinear_interpolation
inline vec3 sample(const Texture *t, vec2 uv) {
    uv.x = uv.x - floor(uv.x);
    uv.y = uv.y - floor(uv.y);
    uv.y = 1.0 - uv.y;

    f32 x = std::min(uv.x * t->width, static_cast<float>(t->width - 1));
    f32 y = std::min(uv.y * t->height, static_cast<float>(t->height - 1));

    i32 x1 = std::min(static_cast<i32>(uv.x * t->width), t->width - 1);
    i32 x2 = std::min(static_cast<i32>(std::ceil(uv.x * t->width)), t->width - 1);

    i32 y1 = std::min(static_cast<i32>(uv.y * t->height), t->height - 1);
    i32 y2 = std::min(static_cast<i32>(std::ceil(uv.y * t->height)), t->height - 1);

    i32 idx11 = (y1 * t->width + x1) * t->channels;
    i32 idx12 = (y2 * t->width + x1) * t->channels;
    i32 idx21 = (y1 * t->width + x2) * t->channels;
    i32 idx22 = (y2 * t->width + x2) * t->channels;

    vec3 Q11 = vec3(t->data[idx11], t->data[idx11 + 1], t->data[idx11 + 2]) / 255.0f;
    vec3 Q12 = vec3(t->data[idx12], t->data[idx12 + 1], t->data[idx12 + 2]) / 255.0f;
    vec3 Q21 = vec3(t->data[idx21], t->data[idx21 + 1], t->data[idx21 + 2]) / 255.0f;
    vec3 Q22 = vec3(t->data[idx22], t->data[idx22 + 1], t->data[idx22 + 2]) / 255.0f;

    f32 denom = (x2-x1)*(y2-y1);
    // TODO FIX THIS
    if (denom < EPS)
        return naive_sample(t, uv);

    f32 w11 = (x2-x)*(y2-y)/denom;
    f32 w12 = (x2-x)*(y-y1)/denom;
    f32 w21 = (x-x1)*(y2-y)/denom;
    f32 w22 = (x-x1)*(y-y1)/denom;

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
