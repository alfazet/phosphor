#include "scene_buffers.hpp"
#include "bounding_box.h"
#include "glm_bundle.hpp"
#include "opencl_ctx.hpp"
#include "texture_meta.h"

cl::Buffer dev_buf(ClContext &ctx, const void *data, u32 count, u32 item_size) {
    return cl::Buffer(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * item_size,
                      const_cast<void *>(data));
}

void SceneBuffers::upload_scene(ClContext &ctx, const SceneData &scene, const std::vector<BvhNode> &bvh) {
    this->n_triangles = scene.triangles.size();
    this->n_materials = scene.materials.size();
    this->n_lights = scene.lights.size();
    this->n_textures = scene.textures.size();

    std::vector<float4> tv0(n_triangles), tv1(n_triangles), tv2(n_triangles);
    std::vector<float2> tuv0(n_triangles), tuv1(n_triangles), tuv2(n_triangles);
    std::vector<float4> tn0(n_triangles), tn1(n_triangles), tn2(n_triangles);
    std::vector<float4> tt0(n_triangles), tt1(n_triangles), tt2(n_triangles);
    std::vector<u32> tmat(n_triangles);
    for (u32 i = 0; i < n_triangles; i++) {
        const auto &t = scene.triangles[i];
        tv0[i] = t.v0;
        tv1[i] = t.v1;
        tv2[i] = t.v2;
        tuv0[i] = t.uv0;
        tuv1[i] = t.uv1;
        tuv2[i] = t.uv2;
        tn0[i] = t.n0;
        tn1[i] = t.n1;
        tn2[i] = t.n2;
        tt0[i] = t.t0;
        tt1[i] = t.t1;
        tt2[i] = t.t2;
        tmat[i] = t.mat_index;
    }

    this->tri_v0 = dev_buf(ctx, tv0.data(), n_triangles, sizeof(float4));
    this->tri_v1 = dev_buf(ctx, tv1.data(), n_triangles, sizeof(float4));
    this->tri_v2 = dev_buf(ctx, tv2.data(), n_triangles, sizeof(float4));
    this->tri_uv0 = dev_buf(ctx, tuv0.data(), n_triangles, sizeof(float2));
    this->tri_uv1 = dev_buf(ctx, tuv1.data(), n_triangles, sizeof(float2));
    this->tri_uv2 = dev_buf(ctx, tuv2.data(), n_triangles, sizeof(float2));
    this->tri_n0 = dev_buf(ctx, tn0.data(), n_triangles, sizeof(float4));
    this->tri_n1 = dev_buf(ctx, tn1.data(), n_triangles, sizeof(float4));
    this->tri_n2 = dev_buf(ctx, tn2.data(), n_triangles, sizeof(float4));
    this->tri_t0 = dev_buf(ctx, tt0.data(), n_triangles, sizeof(float4));
    this->tri_t1 = dev_buf(ctx, tt1.data(), n_triangles, sizeof(float4));
    this->tri_t2 = dev_buf(ctx, tt2.data(), n_triangles, sizeof(float4));
    this->tri_mat_index = dev_buf(ctx, tmat.data(), n_triangles, sizeof(u32));
    this->bvh_nodes = dev_buf(ctx, bvh.data(), bvh.size(), sizeof(BvhNode));
    this->materials = dev_buf(ctx, scene.materials.data(), n_materials, sizeof(Material));
    this->lights = dev_buf(ctx, scene.lights.data(), n_lights, sizeof(Light));

    const std::vector<f32> &pref = scene.light_area_pref_sum;
    this->light_pref_sum = dev_buf(ctx, pref.data(), pref.size(), sizeof(f32));
    this->total_luminance = pref.empty() ? 0.0f : pref.back();

    std::vector<TextureMeta> tex_meta(n_textures);
    std::vector<u8> atlas;
    for (u32 i = 0; i < n_textures; i++) {
        const auto &tex = scene.textures[i];
        TextureMeta m{};
        m.atlas_offset = static_cast<u32>(atlas.size());
        m.width = tex.width;
        m.height = tex.height;
        m.channels = 3;
        m.mip_levels_count = static_cast<u32>(tex.tex_offsets.size());
        m.mip_table_offset = 0;
        tex_meta[i] = m;
        atlas.insert(atlas.end(), tex.tex_atlas.begin(), tex.tex_atlas.end());
    }
    if (atlas.empty())
        atlas.push_back(0);

    this->tex_meta = dev_buf(ctx, tex_meta.data(), tex_meta.size(), sizeof(TextureMeta));
    this->tex_atlas = dev_buf(ctx, atlas.data(), atlas.size(), sizeof(u8));

    BoundingBox scene_bbox = bvh[1].bbox;
    vec3 bbox_min(scene_bbox.bbox_min.x, scene_bbox.bbox_min.y, scene_bbox.bbox_min.z);
    vec3 bbox_max(scene_bbox.bbox_max.x, scene_bbox.bbox_max.y, scene_bbox.bbox_max.z);
    vec3 center = 0.5f * (bbox_min + bbox_max);
    this->scene_center = float4{{center.x, center.y, center.z, 0.0f}};
    this->scene_radius = 0.5f * glm::length(bbox_max - bbox_min);
}

void SceneBuffers::upload_rays(ClContext &ctx, const std::vector<float4> &origins, const std::vector<float4> &dirs) {
    this->n_rays = origins.size();
    this->ray_origin = dev_buf(ctx, origins.data(), n_rays, sizeof(float4));
    this->ray_dir = dev_buf(ctx, dirs.data(), n_rays, sizeof(float4));
}

void SceneBuffers::upload_photons(ClContext &ctx, PhotonHash &hash, std::vector<Photon> &photons) {
    this->n_photons = static_cast<u32>(photons.size());

    photons_sorted = dev_buf(ctx, photons.data(), n_photons, sizeof(Photon));
    tree_index = dev_buf(ctx, hash.tree_index.data(), hash.tree_index.size(), sizeof(u32));
    bucket_tree_offset = dev_buf(ctx, hash.bucket_tree_offset.data(), hash.bucket_tree_offset.size(), sizeof(u32));
    bucket_tree_size = dev_buf(ctx, hash.bucket_tree_size.data(), hash.bucket_tree_size.size(), sizeof(u32));
}

void SceneBuffers::set_emit_photons_args(cl::Kernel &kernel, u32 batch_offset, u32 photons_to_emit, u32 seed,
                                         u32 batch_max_photons, cl::Buffer &out_photons,
                                         cl::Buffer &out_photon_count) const {
    set_kernel_args(kernel, out_photons, out_photon_count, lights, n_lights, batch_max_photons, batch_offset,
                    photons_to_emit, seed, bvh_nodes, tri_v0, tri_v1, tri_v2, tri_n0, tri_n1, tri_n2, tri_uv0, tri_uv1,
                    tri_uv2, tri_t0, tri_t1, tri_t2, tri_mat_index, n_triangles, materials, tex_meta, tex_atlas,
                    light_pref_sum, total_luminance, scene_center, scene_radius);
}

void SceneBuffers::set_trace_rays_args(cl::Kernel &kernel, f32 search_radius, u32 samples, PhotonHashInfo info,
                                       u32 seed, cl::Buffer &out_color) const {
    set_kernel_args(kernel, ray_origin, ray_dir, n_rays, seed, tri_v0, tri_v1, tri_v2, tri_n0, tri_n1, tri_n2, tri_uv0,
                    tri_uv1, tri_uv2, tri_t0, tri_t1, tri_t2, bvh_nodes, tri_mat_index, n_triangles, materials,
                    tex_meta, tex_atlas, photons_sorted, n_photons, search_radius, samples, out_color,
                    tree_index, bucket_tree_offset, bucket_tree_size, info);
}
