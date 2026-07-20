#include "photon_map.hpp"
#include <algorithm>

void PhotonMap::store(const Photon &p) {
    photons_.push_back(p);
    photons_.back().plane = 0;
}

void PhotonMap::build() {
    usize size = 1;
    while (size < photons_.size() + 1)
        size <<= 1;
    kd_tree_.resize(size);
    if (photons_.empty())
        return;

    balance(1, 0, photons_.size());
}

static u8 choose_axis(std::vector<Photon> &photons, usize start, usize end) {
    vec3 minp(INF);
    vec3 maxp(-INF);

    for (usize i = start; i < end; i++) {
        minp = glm::min(minp, photons[i].pos);
        maxp = glm::max(maxp, photons[i].pos);
    }
    vec3 ext = maxp - minp;

    if (ext.x > ext.y && ext.x > ext.z)
        return 0;
    if (ext.y > ext.z)
        return 1;
    return 2;
}

void PhotonMap::balance(usize index, usize start, usize end) {
    if (start >= end || index >= kd_tree_.size())
        return;

    u8 axis = choose_axis(photons_, start, end);
    usize median = (start + end) / 2;

    std::nth_element(photons_.begin() + start, photons_.begin() + median, photons_.begin() + end,
                     [axis](const Photon &a, const Photon &b) { return a.pos[axis] < b.pos[axis]; });

    photons_[median].plane = axis;
    kd_tree_[index] = &photons_[median];

    balance(index * 2, start, median);
    balance(index * 2 + 1, median + 1, end);
}

void PhotonMap::locate(const vec3 &pos, u32 k, f32 max_dist2, std::vector<const Photon *> &result) const {
    result.clear();
    std::vector<f32> dist2;
    locate_rec(1, pos, k, max_dist2, result, dist2);
}

static void try_insert(const Photon *p, f32 d2, u32 k, std::vector<const Photon *> &result, std::vector<f32> &dist2) {
    if ((u32)result.size() < k) {
        result.push_back(p);
        dist2.push_back(d2);
        return;
    }

    u32 worst = 0;
    for (u32 i = 1; i < k; i++) {
        if (dist2[i] > dist2[worst])
            worst = i;
    }

    if (d2 < dist2[worst]) {
        result[worst] = p;
        dist2[worst] = d2;
    }
}

void PhotonMap::locate_rec(usize index, const vec3 &pos, u32 k, f32 max_dist2, std::vector<const Photon *> &result,
                           std::vector<f32> &dist2) const {
    if (index >= kd_tree_.size())
        return;

    Photon *p = kd_tree_[index];
    if (!p)
        return;

    u8 axis = p->plane;
    f32 delta = pos[axis] - p->pos[axis];

    usize near = delta < 0 ? index * 2 : index * 2 + 1;
    usize far = delta < 0 ? index * 2 + 1 : index * 2;

    locate_rec(near, pos, k, max_dist2, result, dist2);

    f32 d2 = glm::dot(p->pos - pos, p->pos - pos);
    if (d2 < max_dist2)
        try_insert(p, d2, k, result, dist2);

    f32 current_max = (result.size() < k) ? max_dist2 : *std::max_element(dist2.begin(), dist2.end());
    if (delta * delta < current_max)
        locate_rec(far, pos, k, max_dist2, result, dist2);
}
