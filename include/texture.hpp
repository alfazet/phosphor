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

inline f32 sample(const Texture *t, vec2 uv, channels ch) {
    uv.x = uv.x - floor(uv.x);
    uv.y = uv.y - floor(uv.y);
    uv.y = 1.0 - uv.y;
    i32 x = std::min((i32)(uv.x * t->width), t->width - 1);
    i32 y = std::min((i32)(uv.y * t->height), t->height - 1);

    i32 idx = (y * t->width + x) * t->channels;
    return t->data[idx + ch] / 255.0f;
}


// TODO: different sampling here
inline vec3 sample(const Texture *t, vec2 uv) {
    uv.x = uv.x - floor(uv.x);
    uv.y = uv.y - floor(uv.y);
    uv.y = 1.0 - uv.y;
    i32 x = std::min((i32)(uv.x * t->width), t->width - 1);
    i32 y = std::min((i32)(uv.y * t->height), t->height - 1);

    i32 idx = (y * t->width + x) * t->channels;
    return vec3(t->data[idx], t->data[idx + 1], t->data[idx + 2]) / 255.0f;
}

inline vec3 normal_sample(const Texture *t, vec2 uv) {
    uv.x = uv.x - floor(uv.x);
    uv.y = uv.y - floor(uv.y);
    uv.y = 1.0 - uv.y;
    i32 x = std::min((i32)(uv.x * t->width), t->width - 1);
    i32 y = std::min((i32)(uv.y * t->height), t->height - 1);

    i32 idx = (y * t->width + x) * t->channels;
    vec3 raw = vec3(t->data[idx], t->data[idx + 1], t->data[idx + 2]) / 255.0f;
    return glm::normalize(raw * 2.0f - 1.0f);
}

#endif // PHOSPHOR_TEXTURE_HPP
