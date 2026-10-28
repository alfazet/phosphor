#include "buffers/geometry.hpp"
#include "host/logger.hpp"

cl::Buffer copy_to_dev_buf(ClContext &ctx, const void *data, u32 count, u32 item_size);

void GeometryBuffers::upload(ClContext &ctx, const SceneData &scene, const Bvh &bvh) {
    n_triangles = scene.triangles.size();
    n_e_triangles = scene.emissive_triangles.size();

    std::vector<TriangleVertices> tv(n_triangles);
    std::vector<TriangleNormals> tn(n_triangles);
    std::vector<TriangleTangents> tt(n_triangles);
    std::vector<TriangleUvs> tuv(n_triangles);
    std::vector<u32> tmat(n_triangles);
    for (u32 i = 0; i < n_triangles; i++) {
        const auto &t = scene.triangles[i];
        tv[i] = {t.v0, t.v1, t.v2};
        tn[i] = {t.n0, t.n1, t.n2};
        tuv[i] = {t.uv0, t.uv1, t.uv2, {}};
        tt[i] = {t.t0, t.t1, t.t2};
        tmat[i] = t.mat_index;
    }

    d_tri_verts = copy_to_dev_buf(ctx, tv.data(), n_triangles, sizeof(TriangleVertices));
    d_tri_normals = copy_to_dev_buf(ctx, tn.data(), n_triangles, sizeof(TriangleNormals));
    d_tri_tangents = copy_to_dev_buf(ctx, tt.data(), n_triangles, sizeof(TriangleTangents));
    d_tri_uvs = copy_to_dev_buf(ctx, tuv.data(), n_triangles, sizeof(TriangleUvs));
    d_tri_mat_index = copy_to_dev_buf(ctx, tmat.data(), n_triangles, sizeof(u32));

    std::vector<TriangleVertices> etv(n_e_triangles);
    std::vector<TriangleNormals> etn(n_e_triangles);
    std::vector<TriangleUvs> etuv(n_e_triangles);
    std::vector<u32> etmat(n_e_triangles);
    for (u32 i = 0; i < n_e_triangles; i++) {
        const auto &t = scene.emissive_triangles[i];
        etv[i] = {t.v0, t.v1, t.v2};
        etn[i] = {t.n0, t.n1, t.n2};
        etuv[i] = {t.uv0, t.uv1, t.uv2, {}};
        etmat[i] = t.mat_index;
    }

    d_etri_verts = copy_to_dev_buf(ctx, etv.data(), n_e_triangles, sizeof(TriangleVertices));
    d_etri_normals = copy_to_dev_buf(ctx, etn.data(), n_e_triangles, sizeof(TriangleNormals));
    d_etri_uvs = copy_to_dev_buf(ctx, etuv.data(), n_e_triangles, sizeof(TriangleUvs));
    d_etri_mat_index = copy_to_dev_buf(ctx, etmat.data(), n_e_triangles, sizeof(u32));

    d_bvh_nodes = copy_to_dev_buf(ctx, bvh.nodes.data(), bvh.nodes.size(), sizeof(BvhNode));

    LOG_INFO("geometry data uploaded: {} triangles, {} emissive triangles, {} BVH nodes", n_triangles, n_e_triangles,
             bvh.nodes.size());
}
