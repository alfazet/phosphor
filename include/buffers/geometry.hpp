#ifndef PHOSPHOR_GEOMETRY_BUFFERS_HPP
#define PHOSPHOR_GEOMETRY_BUFFERS_HPP

#include "common/typedefs.h"
#include "host/bvh.hpp"
#include "host/opencl_ctx.hpp"
#include "host/scene_data.hpp"

struct GeometryBuffers {
    cl::Buffer d_tri_verts;
    cl::Buffer d_tri_normals;
    cl::Buffer d_tri_tangents;
    cl::Buffer d_tri_uvs;
    cl::Buffer d_tri_mat_index;
    u32 n_triangles = 0;

    cl::Buffer d_etri_verts;
    cl::Buffer d_etri_normals;
    cl::Buffer d_etri_uvs;
    cl::Buffer d_etri_mat_index;
    u32 n_e_triangles = 0;

    cl::Buffer d_bvh_nodes;

    void upload(ClContext &ctx, const SceneData &scene, const Bvh &bvh);
};

#endif // PHOSPHOR_GEOMETRY_BUFFERS_HPP
