#include "buffers/rendering.hpp"

cl::Buffer copy_to_dev_buf(ClContext &ctx, const void *data, u32 count, u32 item_size) {
    if (count == 0) {
        // OpenCL rejects zero-byte allocations, and confusingly reports the same error code
        // as "exceeds max alloc size". At least one item should always be allocated.
        u8 dummy[16] = {};
        return cl::Buffer(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, item_size, (void *)(dummy));
    }

    return cl::Buffer(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, count * item_size,
                      const_cast<void *>(data));
}

void RenderingBuffers::upload_scene(ClContext &ctx, const SceneData &scene, const Bvh &bvh) {
    this->geometry_buf.upload(ctx, scene, bvh);
    this->shading_buf.upload(ctx, scene, bvh.get_bbox());
}

void RenderingBuffers::alloc_photon_buffers(ClContext &ctx, u32 max_photons, u32 bucket_count) {
    this->photon_buf.alloc(ctx, max_photons, bucket_count);
}

void RenderingBuffers::alloc_image_buffers(ClContext &ctx, const CameraParams &cam, u32 width, u32 height) {
    this->image_buf.camera = cam;
    this->image_buf.image_width = width;
    this->image_buf.image_height = height;
    this->image_buf.n_pixels = width * height;
    this->image_buf.alloc(ctx);
}

void RenderingBuffers::set_emit_photons_args(cl::Kernel &kernel, u32 batch_offset, u32 photons_to_emit,
                                             u32 max_photon_bounces, u32 seed, u32 batch_max_photons,
                                             cl::Buffer &out_photon_pos, cl::Buffer &out_photon_power,
                                             cl::Buffer &out_photon_dir, cl::Buffer &out_n_photons) const {
    const auto &g = this->geometry_buf;
    const auto &s = this->shading_buf;
    set_kernel_args(kernel, out_photon_pos, out_photon_power, out_photon_dir, out_n_photons, s.d_lights, s.n_lights,
                    batch_max_photons, batch_offset, photons_to_emit, max_photon_bounces, seed, g.d_bvh_nodes,
                    g.d_tri_v0, g.d_tri_v1, g.d_tri_v2, g.d_tri_n0, g.d_tri_n1, g.d_tri_n2, g.d_tri_uv0, g.d_tri_uv1,
                    g.d_tri_uv2, g.d_tri_t0, g.d_tri_t1, g.d_tri_t2, g.d_tri_mat_index, g.n_triangles, g.d_etri_v0,
                    g.d_etri_v1, g.d_etri_v2, g.d_etri_n0, g.d_etri_n1, g.d_etri_n2, g.d_etri_uv0, g.d_etri_uv1,
                    g.d_etri_uv2, g.d_etri_mat_index, s.d_materials, s.d_tex_meta, s.d_tex_atlas, s.d_lum_pref_sum,
                    s.total_lum, s.scene_center, s.scene_radius);
}

void RenderingBuffers::set_trace_rays_args(cl::Kernel &kernel, u32 seed, u32 direct_samples) const {
    const auto &g = this->geometry_buf;
    const auto &s = this->shading_buf;
    const auto &i = this->image_buf;
    set_kernel_args(kernel, i.camera, i.image_width, i.image_height, seed, direct_samples, g.d_tri_v0, g.d_tri_v1,
                    g.d_tri_v2, g.d_tri_n0, g.d_tri_n1, g.d_tri_n2, g.d_tri_uv0, g.d_tri_uv1, g.d_tri_uv2, g.d_tri_t0,
                    g.d_tri_t1, g.d_tri_t2, g.d_bvh_nodes, g.d_tri_mat_index, g.n_triangles, s.d_materials,
                    s.d_tex_meta, s.d_tex_atlas, s.d_lights, s.n_lights, s.d_lum_pref_sum, s.total_lum, s.scene_center,
                    s.scene_radius, g.d_etri_v0, g.d_etri_v1, g.d_etri_v2, g.d_etri_n0, g.d_etri_n1, g.d_etri_n2,
                    g.d_etri_uv0, g.d_etri_uv1, g.d_etri_uv2, g.d_etri_mat_index, i.d_hit_points, s.d_envmap_data,
                    s.envmap_width, s.envmap_height, s.has_envmap);
}

void RenderingBuffers::set_hash_photons_args(cl::Kernel &kernel, cl::Buffer &raw_photon_pos, u32 n_photons_count,
                                             PhotonHashData data) const {
    const auto &p = this->photon_buf;
    set_kernel_args(kernel, raw_photon_pos, p.d_photon_hashes, p.d_cell_counts, n_photons_count, data);
}

void RenderingBuffers::set_scatter_args(cl::Kernel &kernel, cl::Buffer &raw_photon_pos, cl::Buffer &raw_photon_power,
                                        cl::Buffer &raw_photon_dir, u32 n_photons_count) const {
    const auto &p = this->photon_buf;
    set_kernel_args(kernel, raw_photon_pos, raw_photon_power, raw_photon_dir, p.d_photon_hashes, p.d_photon_pos,
                    p.d_photon_power, p.d_photon_dir, p.d_cell_end, n_photons_count);
}

void RenderingBuffers::set_gather_flux_args(cl::Kernel &kernel, PhotonHashData data, f32 sppm_alpha) const {
    const auto &p = this->photon_buf;
    const auto &i = this->image_buf;
    set_kernel_args(kernel, i.d_hit_points, i.d_pixels, i.d_total_radiance, i.n_pixels, p.d_photon_pos,
                    p.d_photon_power, p.d_photon_dir, p.n_photons, p.d_cell_start, p.d_cell_end, data, sppm_alpha);
}
