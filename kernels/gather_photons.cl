float4 gather_photons(__global const Photon *photons, u32 photon_count,
                      float4 hit_pos, float4 normal, f32 search_radius) {
    float4 flux = (float4)(0.0f);
    f32 max_dist_sq = 0.0f;
    f32 radius_sq = search_radius * search_radius;

    for (u32 i = 0; i < photon_count; i++) {
        Photon p = photons[i];
        float4 diff = p.pos - hit_pos;
        f32 dist_sq = dot(diff.xyz, diff.xyz);

        if (dist_sq < radius_sq && dot(p.dir.xyz, normal.xyz) < 0.0f) {
            max_dist_sq = fmax(max_dist_sq, dist_sq);
            flux += p.power;
        }
    }

    f32 area = PI * max_dist_sq;
    return (area > EPS) ? flux / area : (float4)(0.0f);
}

