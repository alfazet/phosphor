#ifndef PHOSPHOR_PHOTON_HPP
#define PHOSPHOR_PHOTON_HPP

#include "common.hpp"

struct Photon {
    vec3 pos;
    vec3 power; // NOTE: possible space saving (page 19)
    f32 phi;
    f32 theta;
    u8 plane; // for kd-tree (splitting axis: 0/1/2)

    Photon(vec3 pos, vec3 power, f32 phi, f32 theta, u8 plane = 0)
        : pos(pos), power(power), phi(phi), theta(theta), plane(plane) {}
};

#endif // PHOSPHOR_PHOTON_HPP
