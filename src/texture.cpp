#include "texture.hpp"

vec2 apply_uv_transform(vec2 uv, const UVTransform &t) {
    vec2 scaled = uv * t.scale;
    f32 c = glm::cos(t.rotation);
    f32 s = glm::sin(t.rotation);
    vec2 rotated(c * scaled.x - s * scaled.y, s * scaled.x + c * scaled.y);

    return rotated + t.offset;
}

void Texture::build_mipmaps() {
    const u8 *src_data = this->data.data();
    i32 w = this->width;
    i32 h = this->height;

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
        this->mip_levels.push_back(dest);
        this->mip_widths.push_back(dest_w);
        this->mip_heights.push_back(dest_h);

        src_data = this->mip_levels.back().data();
        w = dest_w;
        h = dest_h;
    }
}

vec2 Texture::transformed_uv(vec2 uv) const {
    return this->uv_transform.has_value() ? apply_uv_transform(uv, *this->uv_transform) : uv;
}

vec3 Texture::naive_sample(vec2 uv) const {
    uv.x = uv.x - glm::floor(uv.x);
    uv.y = uv.y - glm::floor(uv.y);
    uv.y = 1.0f - uv.y;
    i32 x = glm::min(static_cast<i32>(uv.x * this->width), this->width - 1);
    i32 y = glm::min(static_cast<i32>(uv.y * this->height), this->height - 1);
    i32 idx = (y * this->width + x) * this->channels;

    return vec3(this->data[idx], this->data[idx + 1], this->data[idx + 2]) / 255.0f;
}

vec3 Texture::sample_mip(vec2 uv, i32 level) const {
    uv = this->transformed_uv(uv);
    const u8 *data;
    i32 w, h;
    if (level <= 0 || this->mip_levels.empty()) {
        data = this->data.data();
        w = this->width;
        h = this->height;
    } else {
        i32 clamped = glm::min(level - 1, static_cast<i32>(this->mip_levels.size()) - 1);
        data = this->mip_levels[clamped].data();
        w = this->mip_widths[clamped];
        h = this->mip_heights[clamped];
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

    i32 idx11 = (y1 * w + x1) * this->channels;
    i32 idx12 = (y2 * w + x1) * this->channels;
    i32 idx21 = (y1 * w + x2) * this->channels;
    i32 idx22 = (y2 * w + x2) * this->channels;

    vec3 Q11 = vec3(data[idx11], data[idx11 + 1], data[idx11 + 2]) / 255.0f;
    vec3 Q12 = vec3(data[idx12], data[idx12 + 1], data[idx12 + 2]) / 255.0f;
    vec3 Q21 = vec3(data[idx21], data[idx21 + 1], data[idx21 + 2]) / 255.0f;
    vec3 Q22 = vec3(data[idx22], data[idx22 + 1], data[idx22 + 2]) / 255.0f;

    f32 w11, w12, w21, w22;
    if (x2 == x1 && y2 == y1) {
        return this->naive_sample(uv);
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

vec3 Texture::sample(vec2 uv) const { return this->sample_mip(uv, 0); }

f32 Texture::sample(vec2 uv, TextureChannel ch) const {
    auto sample_all = this->sample_mip(uv, 0);
    return sample_all[ch];
}

vec3 Texture::sample_normal_vec(vec2 uv) const {
    vec3 raw = this->sample_mip(uv, 0);
    return glm::normalize(raw * 2.0f - 1.0f);
}

vec3 Texture::sample_trilinear(vec2 uv, f32 lod) const {
    if (this->mip_levels.empty() || lod <= 0.0f)
        return this->sample_mip(uv, 0);

    i32 max_level = static_cast<i32>(this->mip_levels.size());
    lod = glm::clamp(lod, 0.0f, static_cast<f32>(max_level));

    i32 low = static_cast<i32>(glm::floor(lod));
    i32 high = static_cast<i32>(glm::ceil(lod));
    f32 frac = lod - static_cast<f32>(low);
    if (low == high)
        return this->sample_mip(uv, low);

    vec3 color_low = this->sample_mip(uv, low);
    vec3 color_high = this->sample_mip(uv, high);
    return glm::mix(color_low, color_high, frac);
}

f32 Texture::sample_trilinear(vec2 uv, f32 lod, TextureChannel ch) const { return this->sample_trilinear(uv, lod)[ch]; }
