#include "host/scene_data.hpp"
#include "host/logger.hpp"
#include "stb_image.h"

bool SceneData::is_empty() const { return this->triangles.empty(); }

bool SceneData::has_camera() const { return this->chosen_camera.has_value(); }

Camera &SceneData::get_camera() {
    ASSERT(chosen_camera, "no cameras loaded");
    return this->cameras[*chosen_camera];
}

void SceneData::build_lum_pref_sum() {
    this->lum_pref_sum.clear();
    this->lum_pref_sum.reserve(this->lights.size());
    f32 running = 0.0f;
    for (const auto &l : this->lights) {
        f32 lum = l.power.x * 0.2126f + l.power.y * 0.7152f + l.power.z * 0.0722f;
        running += std::max(lum, 0.0f);
        this->lum_pref_sum.push_back(running);
    }
}

void SceneData::load_envmap(const char *path) {
    this->envmap = EnvMap{};
    i32 w = 0, h = 0;
    f32 *raw = stbi_loadf(path, &w, &h, NULL, 3);
    if (!raw) {
        LOG_ERROR("failed to load envmap {}", path);
        return;
    }
    this->envmap.width = static_cast<u32>(w);
    this->envmap.height = static_cast<u32>(h);
    this->envmap.pixels.assign(raw, raw + 3 * w * h);
    this->envmap.loaded = true;
    stbi_image_free(raw);

    LOG_INFO("loaded envmap {}", path);
}
