#ifndef PHOSPHOR_GEOMETRY_BUFFERS_HPP
#define PHOSPHOR_GEOMETRY_BUFFERS_HPP

#include "common/typedefs.h"
#include "host/bvh.hpp"
#include "host/opencl_ctx.hpp"
#include "host/scene_data.hpp"

struct GeometryBuffers {
    cl::Buffer d_tri_verts;
    cl::Buffer d_tri_uv0, d_tri_uv1, d_tri_uv2;
    cl::Buffer d_tri_n0, d_tri_n1, d_tri_n2;
    cl::Buffer d_tri_t0, d_tri_t1, d_tri_t2;
    cl::Buffer d_tri_mat_index;
    u32 n_triangles = 0;

    cl::Buffer d_etri_verts;
    cl::Buffer d_etri_uv0, d_etri_uv1, d_etri_uv2;
    cl::Buffer d_etri_n0, d_etri_n1, d_etri_n2;
    cl::Buffer d_etri_mat_index;
    u32 n_e_triangles = 0;

    cl::Buffer d_bvh_nodes;

    void upload(ClContext &ctx, const SceneData &scene, const Bvh &bvh);
};

#endif // PHOSPHOR_GEOMETRY_BUFFERS_HPP
