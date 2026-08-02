#ifndef PHOSPHOR_TEXTURE_HPP
#define PHOSPHOR_TEXTURE_HPP

#include "common.hpp"
#include "glm/gtx/raw_data.hpp"
#include "ray.hpp"
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
    std::vector<u8> data; // original (mipmap level 0)
    std::vector<std::vector<u8>> mip_levels;
    std::vector<i32> mip_widths;
    std::vector<i32> mip_heights;

    std::optional<UVTransform> uv_transform;
};

inline void build_mipmaps(Texture &t) {
    const u8 *src_data = t.data.data();
    i32 w = t.width;
    i32 h = t.height;

    while (w > 1 || h > 1) {
        i32 dest_w = glm::max(w / 2, 1);
        i32 dest_h = glm::max(h / 2, 1);
        std::vector<u8> dest(dest_w * dest_h * 3);

        for (i32 y = 0; y < dest_h; y++) {
            for (i32 x = 0; x < dest_w; x++) {
                i32 sx0 = glm::min(x * 2, w - 1);
                i32 sx1 = glm::min(x * 2 + 1, w - 1);
                i32 sy0 = glm::min(y * 2, h - 1);
                i32 sy1 = glm::min(y * 2 + 1, h - 1);

                for (i32 chan = 0; chan < 3; chan++) {
                    u32 s = static_cast<u32>(src_data[(sy0 * w + sx0) * 3 + chan]) +
                            static_cast<u32>(src_data[(sy0 * w + sx1) * 3 + chan]) +
                            static_cast<u32>(src_data[(sy1 * w + sx0) * 3 + chan]) +
                            static_cast<u32>(src_data[(sy1 * w + sx1) * 3 + chan]);
                    dest[(y * dest_w + x) * 3 + chan] = static_cast<u8>(s / 4);
                }
            }
        }
        t.mip_levels.push_back(dest);
        t.mip_widths.push_back(dest_w);
        t.mip_heights.push_back(dest_h);

        src_data = t.mip_levels.back().data();
        w = dest_w;
        h = dest_h;
    }
}

enum channels { CHANNEL_R, CHANNEL_G, CHANNEL_B };

namespace texture {

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
    uv.x = uv.x - glm::floor(uv.x);
    uv.y = uv.y - glm::floor(uv.y);
    uv.y = 1.0f - uv.y;
    i32 x = glm::min(static_cast<i32>(uv.x * t->width), t->width - 1);
    i32 y = glm::min(static_cast<i32>(uv.y * t->height), t->height - 1);
    i32 idx = (y * t->width + x) * t->channels;

    return vec3(t->data[idx], t->data[idx + 1], t->data[idx + 2]) / 255.0f;
}

inline vec3 sample_mip(const Texture *t, vec2 uv, i32 level) {
    uv = transformed_uv(t, uv);
    const u8 *data;
    i32 w, h;
    if (level <= 0 || t->mip_levels.empty()) {
        data = t->data.data();
        w = t->width;
        h = t->height;
    } else {
        i32 clamped = glm::min(level - 1, static_cast<i32>(t->mip_levels.size()) - 1);
        data = t->mip_levels[clamped].data();
        w = t->mip_widths[clamped];
        h = t->mip_heights[clamped];
    }

    uv.x = uv.x - glm::floor(uv.x);
    uv.y = uv.y - glm::floor(uv.y);
    uv.y = 1.0f - uv.y;

    f32 x = glm::min(uv.x * w, static_cast<f32>(w - 1));
    f32 y = glm::min(uv.y * h, static_cast<f32>(h - 1));
    i32 x1 = glm::min(static_cast<i32>(x), w - 1);
    i32 x2 = glm::min(static_cast<i32>(glm::ceil(x)), w - 1);
    i32 y1 = glm::min(static_cast<i32>(y), h - 1);
    i32 y2 = glm::min(static_cast<i32>(glm::ceil(y)), h - 1);

    i32 idx11 = (y1 * w + x1) * t->channels;
    i32 idx12 = (y2 * w + x1) * t->channels;
    i32 idx21 = (y1 * w + x2) * t->channels;
    i32 idx22 = (y2 * w + x2) * t->channels;

    vec3 Q11 = vec3(data[idx11], data[idx11 + 1], data[idx11 + 2]) / 255.0f;
    vec3 Q12 = vec3(data[idx12], data[idx12 + 1], data[idx12 + 2]) / 255.0f;
    vec3 Q21 = vec3(data[idx21], data[idx21 + 1], data[idx21 + 2]) / 255.0f;
    vec3 Q22 = vec3(data[idx22], data[idx22 + 1], data[idx22 + 2]) / 255.0f;

    f32 w11, w12, w21, w22;
    if (x2 == x1 && y2 == y1) {
        return naive_sample(t, uv);
    } else if (x2 == x1) {
        return glm::mix(Q11, Q12, y - y1);
    } else if (y2 == y1) {
        return glm::mix(Q11, Q21, x - x1);
    } else {
        f32 denom = (x2 - x1) * (y2 - y1);
        w11 = (x2 - x) * (y2 - y) / denom;
        w12 = (x2 - x) * (y - y1) / denom;
        w21 = (x - x1) * (y2 - y) / denom;
        w22 = (x - x1) * (y - y1) / denom;
    }

    return w11 * Q11 + w12 * Q12 + w21 * Q21 + w22 * Q22;
}

inline vec3 sample(const Texture *t, vec2 uv) { return sample_mip(t, uv, 0); }

inline f32 sample(const Texture *t, vec2 uv, channels ch) {
    auto sample_all = sample_mip(t, uv, 0);
    return sample_all[ch];
}

inline vec3 normal_sample(const Texture *t, vec2 uv) {
    vec3 raw = sample_mip(t, uv, 0);
    return glm::normalize(raw * 2.0f - 1.0f);
}

inline vec3 sample_trilinear(const Texture *t, vec2 uv, f32 lod) {
    if (t->mip_levels.empty() || lod <= 0.0f)
        return sample_mip(t, uv, 0);

    i32 max_level = static_cast<i32>(t->mip_levels.size());
    lod = glm::clamp(lod, 0.0f, static_cast<f32>(max_level));

    i32 low = static_cast<i32>(glm::floor(lod));
    i32 high = static_cast<i32>(glm::ceil(lod));
    f32 frac = lod - static_cast<f32>(low);
    if (low == high)
        return sample_mip(t, uv, low);

    vec3 color_low = sample_mip(t, uv, low);
    vec3 color_high = sample_mip(t, uv, high);
    return glm::mix(color_low, color_high, frac);
}

inline f32 sample_trilinear(const Texture *t, vec2 uv, f32 lod, channels ch) {
    return sample_trilinear(t, uv, lod)[ch];
}

inline f32 compute_uv_lod(const Ray &r, f32 t_hit, const vec3 &normal, const vec3 &tangent, const vec3 &bitangent,
                          i32 tex_width, i32 tex_height) {
    f32 denom = glm::dot(r.direction, normal);
    if (glm::abs(denom) < EPS)
        return 0.0f;

    f32 dt_x = -glm::dot(r.dp_dx, normal) / denom;
    f32 dt_y = -glm::dot(r.dp_dy, normal) / denom;
    vec3 dpx = r.dp_dx + dt_x * r.direction + t_hit * r.dd_dx;
    vec3 dpy = r.dp_dy + dt_y * r.direction + t_hit * r.dd_dy;

    vec2 duvx = vec2(glm::dot(tangent, dpx), glm::dot(bitangent, dpx));
    vec2 duvy = vec2(glm::dot(tangent, dpy), glm::dot(bitangent, dpy));
    f32 len_x = glm::length(duvx * vec2(tex_width, tex_height));
    f32 len_y = glm::length(duvy * vec2(tex_width, tex_height));

    f32 len_max = glm::max(len_x, len_y);
    if (len_max < EPS)
        return 0.0f;

    return glm::max(0.0f, glm::log2(len_max));
}

} // namespace texture

#endif // PHOSPHOR_TEXTURE_HPP
