#include "buffers/geometry.hpp"
#include "host/logger.hpp"

cl::Buffer copy_to_dev_buf(ClContext &ctx, const void *data, u32 count, u32 item_size);

void GeometryBuffers::upload(ClContext &ctx, const SceneData &scene, const Bvh &bvh) {
    n_triangles = scene.triangles.size();
    n_e_triangles = scene.emissive_triangles.size();

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

    d_tri_v0 = copy_to_dev_buf(ctx, tv0.data(), n_triangles, sizeof(float4));
    d_tri_v1 = copy_to_dev_buf(ctx, tv1.data(), n_triangles, sizeof(float4));
    d_tri_v2 = copy_to_dev_buf(ctx, tv2.data(), n_triangles, sizeof(float4));
    d_tri_uv0 = copy_to_dev_buf(ctx, tuv0.data(), n_triangles, sizeof(float2));
    d_tri_uv1 = copy_to_dev_buf(ctx, tuv1.data(), n_triangles, sizeof(float2));
    d_tri_uv2 = copy_to_dev_buf(ctx, tuv2.data(), n_triangles, sizeof(float2));
    d_tri_n0 = copy_to_dev_buf(ctx, tn0.data(), n_triangles, sizeof(float4));
    d_tri_n1 = copy_to_dev_buf(ctx, tn1.data(), n_triangles, sizeof(float4));
    d_tri_n2 = copy_to_dev_buf(ctx, tn2.data(), n_triangles, sizeof(float4));
    d_tri_t0 = copy_to_dev_buf(ctx, tt0.data(), n_triangles, sizeof(float4));
    d_tri_t1 = copy_to_dev_buf(ctx, tt1.data(), n_triangles, sizeof(float4));
    d_tri_t2 = copy_to_dev_buf(ctx, tt2.data(), n_triangles, sizeof(float4));
    d_tri_mat_index = copy_to_dev_buf(ctx, tmat.data(), n_triangles, sizeof(u32));

    std::vector<float4> etv0(n_e_triangles), etv1(n_e_triangles), etv2(n_e_triangles);
    std::vector<float2> etuv0(n_e_triangles), etuv1(n_e_triangles), etuv2(n_e_triangles);
    std::vector<float4> etn0(n_e_triangles), etn1(n_e_triangles), etn2(n_e_triangles);
    std::vector<float4> ett0(n_e_triangles), ett1(n_e_triangles), ett2(n_e_triangles);
    std::vector<u32> etmat(n_e_triangles);
    for (u32 i = 0; i < n_e_triangles; i++) {
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

    d_etri_v0 = copy_to_dev_buf(ctx, etv0.data(), n_e_triangles, sizeof(float4));
    d_etri_v1 = copy_to_dev_buf(ctx, etv1.data(), n_e_triangles, sizeof(float4));
    d_etri_v2 = copy_to_dev_buf(ctx, etv2.data(), n_e_triangles, sizeof(float4));
    d_etri_uv0 = copy_to_dev_buf(ctx, etuv0.data(), n_e_triangles, sizeof(float2));
    d_etri_uv1 = copy_to_dev_buf(ctx, etuv1.data(), n_e_triangles, sizeof(float2));
    d_etri_uv2 = copy_to_dev_buf(ctx, etuv2.data(), n_e_triangles, sizeof(float2));
    d_etri_n0 = copy_to_dev_buf(ctx, etn0.data(), n_e_triangles, sizeof(float4));
    d_etri_n1 = copy_to_dev_buf(ctx, etn1.data(), n_e_triangles, sizeof(float4));
    d_etri_n2 = copy_to_dev_buf(ctx, etn2.data(), n_e_triangles, sizeof(float4));
    d_etri_t0 = copy_to_dev_buf(ctx, ett0.data(), n_e_triangles, sizeof(float4));
    d_etri_t1 = copy_to_dev_buf(ctx, ett1.data(), n_e_triangles, sizeof(float4));
    d_etri_t2 = copy_to_dev_buf(ctx, ett2.data(), n_e_triangles, sizeof(float4));
    d_etri_mat_index = copy_to_dev_buf(ctx, etmat.data(), n_e_triangles, sizeof(u32));
    d_bvh_nodes = copy_to_dev_buf(ctx, bvh.nodes.data(), bvh.nodes.size(), sizeof(BvhNode));

    LOG_INFO("geometry data uploaded: {} triangles, {} emissive triangles, {} BVH nodes", n_triangles, n_e_triangles,
             bvh.nodes.size());
}
