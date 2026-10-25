#include "buffers/shading.hpp"
#include "common/bounding_box.h"
#include "common/texture_meta.h"
#include "host/glm_bundle.hpp"
#include "host/logger.hpp"

cl::Buffer copy_to_dev_buf(ClContext &ctx, const void *data, u32 count, u32 item_size);

void ShadingBuffers::upload(ClContext &ctx, const SceneData &scene, const BoundingBox &scene_bbox) {
    u32 n_materials = scene.materials.size();
    u32 n_lights = scene.lights.size();
    u32 n_textures = scene.textures.size();
    this->n_materials = n_materials;
    this->n_lights = n_lights;
    this->n_textures = n_textures;

    this->d_materials = copy_to_dev_buf(ctx, scene.materials.data(), n_materials, sizeof(Material));
    this->d_lights = copy_to_dev_buf(ctx, scene.lights.data(), n_lights, sizeof(Light));

    const std::vector<f32> &lum_pref_sum = scene.lum_pref_sum;
    this->d_lum_pref_sum = copy_to_dev_buf(ctx, lum_pref_sum.data(), lum_pref_sum.size(), sizeof(f32));
    this->total_lum = lum_pref_sum.empty() ? 0.0f : lum_pref_sum.back();

    std::vector<TextureMeta> tex_meta_vec(n_textures);
    std::vector<u8> atlas;
    for (u32 i = 0; i < n_textures; i++) {
        const auto &tex = scene.textures[i];
        TextureMeta m{};
        m.atlas_offset = static_cast<u32>(atlas.size());
        m.width = tex.width;
        m.height = tex.height;
        m.channels = 3;
        tex_meta_vec[i] = m;
        atlas.insert(atlas.end(), tex.tex_atlas.begin(), tex.tex_atlas.end());
    }
    if (atlas.empty())
        atlas.push_back(0);
    if (tex_meta_vec.empty())
        tex_meta_vec.push_back(TextureMeta{});

    this->d_tex_meta = copy_to_dev_buf(ctx, tex_meta_vec.data(), tex_meta_vec.size(), sizeof(TextureMeta));
    this->d_tex_atlas = copy_to_dev_buf(ctx, atlas.data(), atlas.size(), sizeof(u8));

    vec3 bbox_min(scene_bbox.bbox_min.x, scene_bbox.bbox_min.y, scene_bbox.bbox_min.z);
    vec3 bbox_max(scene_bbox.bbox_max.x, scene_bbox.bbox_max.y, scene_bbox.bbox_max.z);
    vec3 center = 0.5f * (bbox_min + bbox_max);
    this->scene_center = float4{{center.x, center.y, center.z, 0.0f}};
    const vec3 corners[8] = {
        {scene_bbox.bbox_min.x, scene_bbox.bbox_min.y, scene_bbox.bbox_min.z},
        {scene_bbox.bbox_max.x, scene_bbox.bbox_min.y, scene_bbox.bbox_min.z},
        {scene_bbox.bbox_min.x, scene_bbox.bbox_max.y, scene_bbox.bbox_min.z},
        {scene_bbox.bbox_min.x, scene_bbox.bbox_min.y, scene_bbox.bbox_max.z},
        {scene_bbox.bbox_max.x, scene_bbox.bbox_max.y, scene_bbox.bbox_min.z},
        {scene_bbox.bbox_min.x, scene_bbox.bbox_max.y, scene_bbox.bbox_max.z},
        {scene_bbox.bbox_max.x, scene_bbox.bbox_min.y, scene_bbox.bbox_max.z},
        {scene_bbox.bbox_max.x, scene_bbox.bbox_max.y, scene_bbox.bbox_max.z},
    };
    f32 radius = 0.0f;
    for (const auto &corner : corners)
        radius = glm::max(radius, glm::length(corner - center));
    this->scene_radius = radius;

    if (scene.envmap.loaded) {
        this->has_envmap = 1;
        this->envmap_width = scene.envmap.width;
        this->envmap_height = scene.envmap.height;
        this->d_envmap_data =
            copy_to_dev_buf(ctx, scene.envmap.pixels.data(), 3 * envmap_width * envmap_height, sizeof(f32));
    } else {
        this->has_envmap = 0;
        this->envmap_width = 0;
        this->envmap_height = 0;
        this->d_envmap_data = copy_to_dev_buf(ctx, nullptr, 0, sizeof(u8));
    }

    LOG_INFO("shading data uploaded: {} materials, {} lights, {} textures", n_materials, n_lights, n_textures);
}
