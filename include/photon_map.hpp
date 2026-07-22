#ifndef PHOSPHOR_PHOTON_MAP_HPP
#define PHOSPHOR_PHOTON_MAP_HPP

#include "common.hpp"
#include "photon.hpp"

class PhotonMap {
  public:

    void init_thread_buffers(u32 thread_count);
    void store(u32 thread_id, const Photon &p);
    void merge_thread_buffers();

    void build();
    void locate(const vec3 &pos, u32 k, f32 max_dist2, std::vector<const Photon *> &result) const;

  private:
    std::vector<std::vector<Photon>> thread_photons_;
    std::vector<Photon> photons_;
    std::vector<Photon *> kd_tree_; // 1-indexed

    void balance(usize index, usize start, usize end);
    void locate_rec(usize index, const vec3 &pos, u32 k, f32 max_dist2, std::vector<const Photon *> &result,
                    std::vector<f32> &dist2) const;
};

#endif // PHOSPHOR_PHOTON_MAP_HPP
