#ifndef PHOSPHOR_PHOTONMAP_HPP
#define PHOSPHOR_PHOTONMAP_HPP

#include "common.hpp"
#include "photon.hpp"

class PhotonMap {
  public:
    void store(const Photon &p);
    void build();
    void locate(const vec3 &pos, u32 k, f32 max_dist2, std::vector<const Photon *> &result) const;

  private:
    std::vector<Photon> photons_;
    std::vector<Photon *> kd_tree_; // 1-indexed

    void balance(usize index, usize start, usize end);
    void locate_rec(usize index, const vec3 &pos, u32 k, f32 max_dist2, std::vector<const Photon *> &result,
                    std::vector<f32> &dist2) const;
};

#endif // PHOSPHOR_PHOTONMAP_HPP
