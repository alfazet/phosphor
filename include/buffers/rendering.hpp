#ifndef PHOSPHOR_RENDERING_BUFFERS_HPP
#define PHOSPHOR_RENDERING_BUFFERS_HPP

#include "buffers/geometry.hpp"
#include "buffers/image.hpp"
#include "buffers/photon.hpp"
#include "buffers/shading.hpp"
#include "common/camera.h"
#include "common/photon_hash.h"
#include "common/typedefs.h"
#include "host/bvh.hpp"
#include "host/opencl_ctx.hpp"
#include "host/scene_data.hpp"

cl::Buffer copy_to_dev_buf(ClContext &ctx, const void *data, u32 count, u32 item_size);

struct RenderingBuffers {
    GeometryBuffers geometry_buf;
    ShadingBuffers shading_buf;
    PhotonBuffers photon_buf;
    ImageBuffers image_buf;

    void upload_scene(ClContext &ctx, const SceneData &scene, const Bvh &bvh);

    void alloc_photon_buffers(ClContext &ctx, u32 max_photons, u32 bucket_count);

    void alloc_image_buffers(ClContext &ctx, const CameraParams &cam, u32 width, u32 height);

    u32 n_pixels() const { return this->image_buf.n_pixels; }

    u32 &n_photons() { return this->photon_buf.n_photons; }

    u32 n_photons() const { return this->photon_buf.n_photons; }

    cl::Buffer &d_flux() { return this->image_buf.d_flux; }

    cl::Buffer &d_radiance() { return this->image_buf.d_radiance; }

    void set_trace_rays_args(cl::Kernel &kernel, u32 seed, u32 direct_samples) const;

    void set_emit_photons_args(cl::Kernel &kernel, u32 batch_offset, u32 photons_to_emit, u32 max_photon_bounces,
                               u32 seed, u32 max_photons, cl::Buffer &out_photon_pos, cl::Buffer &out_photon_power,
                               cl::Buffer &out_photon_dir, cl::Buffer &out_n_photons) const;

    void set_hash_photons_args(cl::Kernel &kernel, const cl::Buffer &photon_pos, u32 n_photons,
                               PhotonHashData data) const;

    void set_scatter_args(cl::Kernel &kernel, cl::Buffer &photon_pos, cl::Buffer &photon_power, cl::Buffer &photon_dir,
                          u32 n_photons) const;

    void set_gather_flux_args(cl::Kernel &kernel, PhotonHashData data, f32 sppm_alpha) const;
};

#endif // PHOSPHOR_RENDERING_BUFFERS_HPP
