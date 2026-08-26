#include "scene_buffers.hpp"
#include "bounding_box.h"
#include "glm_bundle.hpp"
#include "logger.hpp"
#include "opencl_ctx.hpp"
#include "texture_meta.h"

cl::Buffer dev_buf(ClContext &ctx, const void *data, u32 count, u32 item_size) {
    if (count == 0) {
        static const u8 dummy[16] = {};
        return cl::Buffer(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, item_size, (void *)(dummy));
    }

    return cl::Buffer(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * item_size,
                      const_cast<void *>(data));
}

void SceneBuffers::upload_scene(ClContext &ctx, const SceneData &scene, const Bvh &bvh) {
    this->n_triangles = scene.triangles.size();
    this->en_triangles = scene.emissive_triangles.size();
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

    std::vector<float4> etv0(en_triangles), etv1(en_triangles), etv2(en_triangles);
    std::vector<float2> etuv0(en_triangles), etuv1(en_triangles), etuv2(en_triangles);
    std::vector<float4> etn0(en_triangles), etn1(en_triangles), etn2(en_triangles);
    std::vector<float4> ett0(en_triangles), ett1(en_triangles), ett2(en_triangles);
    std::vector<u32> etmat(en_triangles);
    for (u32 i = 0; i < en_triangles; i++) {
        const auto &t = scene.emissive_triangles[i];
        etv0[i] = t.v0;
        etv1[i] = t.v1;
        etv2[i] = t.v2;
        etuv0[i] = t.uv0;
        etuv1[i] = t.uv1;
        etuv2[i] = t.uv2;
        etn0[i] = t.n0;
        etn1[i] = t.n1;
        etn2[i] = t.n2;
        ett0[i] = t.t0;
        ett1[i] = t.t1;
        ett2[i] = t.t2;
        etmat[i] = t.mat_index;
    }

    this->etri_v0 = dev_buf(ctx, etv0.data(), en_triangles, sizeof(float4));
    this->etri_v1 = dev_buf(ctx, etv1.data(), en_triangles, sizeof(float4));
    this->etri_v2 = dev_buf(ctx, etv2.data(), en_triangles, sizeof(float4));
    this->etri_uv0 = dev_buf(ctx, etuv0.data(), en_triangles, sizeof(float2));
    this->etri_uv1 = dev_buf(ctx, etuv1.data(), en_triangles, sizeof(float2));
    this->etri_uv2 = dev_buf(ctx, etuv2.data(), en_triangles, sizeof(float2));
    this->etri_n0 = dev_buf(ctx, etn0.data(), en_triangles, sizeof(float4));
    this->etri_n1 = dev_buf(ctx, etn1.data(), en_triangles, sizeof(float4));
    this->etri_n2 = dev_buf(ctx, etn2.data(), en_triangles, sizeof(float4));
    this->etri_t0 = dev_buf(ctx, ett0.data(), en_triangles, sizeof(float4));
    this->etri_t1 = dev_buf(ctx, ett1.data(), en_triangles, sizeof(float4));
    this->etri_t2 = dev_buf(ctx, ett2.data(), en_triangles, sizeof(float4));
    this->etri_mat_index = dev_buf(ctx, etmat.data(), en_triangles, sizeof(u32));

    this->bvh_nodes = dev_buf(ctx, bvh.nodes.data(), bvh.nodes.size(), sizeof(BvhNode));
    this->materials = dev_buf(ctx, scene.materials.data(), n_materials, sizeof(Material));
    this->lights = dev_buf(ctx, scene.lights.data(), n_lights, sizeof(Light));

    const std::vector<f32> &luminance_pref_sum = scene.luminance_pref_sum;
    this->light_pref_sum = dev_buf(ctx, luminance_pref_sum.data(), luminance_pref_sum.size(), sizeof(f32));
    this->total_luminance = luminance_pref_sum.empty() ? 0.0f : luminance_pref_sum.back();

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
    if (tex_meta.empty())
        tex_meta.push_back(TextureMeta{});
    if (atlas.empty())
        atlas.push_back(0);

    this->tex_meta = dev_buf(ctx, tex_meta.data(), tex_meta.size(), sizeof(TextureMeta));
    this->tex_atlas = dev_buf(ctx, atlas.data(), atlas.size(), sizeof(u8));

    BoundingBox scene_bbox = bvh.get_bbox();
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
    for (const auto &corner : corners) {
        vec3 offset = corner - center;
        radius = glm::max(radius, glm::length(offset));
    }
    this->scene_radius = radius;
}

void SceneBuffers::upload_rays(ClContext &ctx, const std::vector<float4> &origins, const std::vector<float4> &dirs) {
    this->n_rays = origins.size();
    this->ray_origin = dev_buf(ctx, origins.data(), n_rays, sizeof(float4));
    this->ray_dir = dev_buf(ctx, dirs.data(), n_rays, sizeof(float4));
}

void SceneBuffers::upload_photons(ClContext &ctx, PhotonHash &hash, std::vector<float4> &photon_pos,
                                  std::vector<float4> &photon_power, std::vector<float4> &photon_dir,
                                  std::vector<float4> &photon_normal) {
    this->n_photons = static_cast<u32>(photon_pos.size());

    this->photon_pos = dev_buf(ctx, photon_pos.data(), n_photons, sizeof(float4));
    this->photon_power = dev_buf(ctx, photon_power.data(), n_photons, sizeof(float4));
    this->photon_dir = dev_buf(ctx, photon_dir.data(), n_photons, sizeof(float4));
    this->photon_normal = dev_buf(ctx, photon_normal.data(), n_photons, sizeof(float4));

    tree_index = dev_buf(ctx, hash.tree_index.data(), hash.tree_index.size(), sizeof(u32));
    bucket_tree_offset = dev_buf(ctx, hash.bucket_tree_offset.data(), hash.bucket_tree_offset.size(), sizeof(u32));
    bucket_tree_size = dev_buf(ctx, hash.bucket_tree_size.data(), hash.bucket_tree_size.size(), sizeof(u32));
}

void SceneBuffers::set_emit_photons_args(cl::Kernel &kernel, u32 batch_offset, u32 photons_to_emit, u32 seed,
                                         u32 batch_max_photons, cl::Buffer &out_photon_pos,
                                         cl::Buffer &out_photon_power, cl::Buffer &out_photon_dir,
                                         cl::Buffer &out_photon_normal, cl::Buffer &out_photon_count) const {
    set_kernel_args(kernel, out_photon_pos, out_photon_power, out_photon_dir, out_photon_normal, out_photon_count,
                    lights, n_lights, batch_max_photons, batch_offset, photons_to_emit, seed, bvh_nodes, tri_v0, tri_v1,
                    tri_v2, tri_n0, tri_n1, tri_n2, tri_uv0, tri_uv1, tri_uv2, tri_t0, tri_t1, tri_t2, tri_mat_index,
                    n_triangles, etri_v0, etri_v1, etri_v2, etri_n0, etri_n1, etri_n2, etri_uv0, etri_uv1, etri_uv2,
                    etri_t0, etri_t1, etri_t2, etri_mat_index, materials, tex_meta, tex_atlas, light_pref_sum,
                    total_luminance, scene_center, scene_radius);
}

void SceneBuffers::set_trace_rays_args(cl::Kernel &kernel, f32 search_radius, u32 samples, PhotonHashInfo info,
                                       u32 seed, cl::Buffer &out_color) const {
    set_kernel_args(kernel, ray_origin, ray_dir, n_rays, seed, tri_v0, tri_v1, tri_v2, tri_n0, tri_n1, tri_n2, tri_uv0,
                    tri_uv1, tri_uv2, tri_t0, tri_t1, tri_t2, bvh_nodes, tri_mat_index, n_triangles, materials,
                    tex_meta, tex_atlas, photon_pos, photon_power, photon_dir, photon_normal, n_photons, search_radius,
                    samples, out_color, tree_index, bucket_tree_offset, bucket_tree_size, info, lights, n_lights,
                    light_pref_sum, total_luminance, scene_center, scene_radius, etri_v0, etri_v1, etri_v2, etri_n0,
                    etri_n1, etri_n2, etri_uv0, etri_uv1, etri_uv2);
}

void SceneBuffers::print_buffer_sizes() const {
    auto sz = [](const char *name, const cl::Buffer &buf) {
        if (buf() == nullptr) {
            LOG_INFO("{:<24} : (unallocated)", name);
            return;
        }
        cl_ulong bytes = buf.getInfo<CL_MEM_SIZE>();
        LOG_INFO("{:<24} : {} bytes", name, bytes);
    };

    sz("tri_v0", tri_v0);
    sz("tri_v1", tri_v1);
    sz("tri_v2", tri_v2);
    sz("tri_uv0", tri_uv0);
    sz("tri_uv1", tri_uv1);
    sz("tri_uv2", tri_uv2);
    sz("tri_n0", tri_n0);
    sz("tri_n1", tri_n1);
    sz("tri_n2", tri_n2);
    sz("tri_t0", tri_t0);
    sz("tri_t1", tri_t1);
    sz("tri_t2", tri_t2);
    sz("tri_mat_index", tri_mat_index);

    sz("etri_v0", etri_v0);
    sz("etri_v1", etri_v1);
    sz("etri_v2", etri_v2);
    sz("etri_uv0", etri_uv0);
    sz("etri_uv1", etri_uv1);
    sz("etri_uv2", etri_uv2);
    sz("etri_n0", etri_n0);
    sz("etri_n1", etri_n1);
    sz("etri_n2", etri_n2);
    sz("etri_t0", etri_t0);
    sz("etri_t1", etri_t1);
    sz("etri_t2", etri_t2);
    sz("etri_mat_index", etri_mat_index);

    sz("bvh_nodes", bvh_nodes);
    sz("materials", materials);
    sz("lights", lights);
    sz("light_pref_sum", light_pref_sum);

    sz("tex_meta", tex_meta);
    sz("tex_atlas", tex_atlas);

    sz("ray_origin", ray_origin);
    sz("ray_dir", ray_dir);

    sz("photon_pos", photon_pos);
    sz("photon_power", photon_power);
    sz("photon_dir", photon_dir);
    sz("photon_normal", photon_normal);
    sz("tree_index", tree_index);
    sz("bucket_tree_offset", bucket_tree_offset);
    sz("bucket_tree_size", bucket_tree_size);
}
