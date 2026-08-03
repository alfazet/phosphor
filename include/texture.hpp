#ifndef PHOSPHOR_TEXTURE_HPP
#define PHOSPHOR_TEXTURE_HPP

#include "common.hpp"
#include "glm/gtx/raw_data.hpp"
#include "stb_image.h"

#include <optional>

enum TextureChannel { CHANNEL_R, CHANNEL_G, CHANNEL_B };

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
    std::vector<u8> data; // original (mipmap level 0)
    std::vector<std::vector<u8>> mip_levels;
    std::vector<i32> mip_widths;
    std::vector<i32> mip_heights;

    std::optional<UVTransform> uv_transform;

    void build_mipmaps();

    vec2 transformed_uv(vec2 uv) const;

    vec3 naive_sample(vec2 uv) const;

    vec3 sample_mip(vec2 uv, i32 level) const;

    vec3 sample(vec2 uv) const;

    f32 sample(vec2 uv, TextureChannel ch) const;

    vec3 sample_trilinear(vec2 uv, f32 lod) const;

    f32 sample_trilinear(vec2 uv, f32 lod, TextureChannel ch) const;

    vec3 sample_normal_vec(vec2 uv) const;
};

#endif // PHOSPHOR_TEXTURE_HPP
