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

inline Texture load(const std::string &path) {
    i32 w, h, c;
    unsigned char *raw = stbi_load(path.c_str(), &w, &h, &c, 3);
    if (!raw)
        throw std::runtime_error("failed to load texture from " + path);

    Texture t;
    t.name = std::filesystem::path(path).filename().string();
    t.width = w;
    t.height = h;
    t.channels = 3;
    t.data.assign(raw, raw + (w * h * 3));
    stbi_image_free(raw);
    return t;
}

inline vec3 sample(const Texture *t, vec2 uv) {
    uv.x = uv.x - floor(uv.x);
    uv.y = uv.y - floor(uv.y);
    uv.y = 1.0 - uv.y;
    i32 x = std::min((i32)(uv.x * t->width), t->width - 1);
    i32 y = std::min((i32)(uv.y * t->height), t->height - 1);

    i32 idx = (y * t->width + x) * t->channels;
    return vec3(t->data[idx], t->data[idx + 1], t->data[idx + 2]) / 255.0f;
}

#endif // PHOSPHOR_TEXTURE_HPP
