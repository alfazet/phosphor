#ifndef PHOSPHOR_PHOTON_HPP
#define PHOSPHOR_PHOTON_HPP

#include "common.hpp"

struct Photon {
    vec3 pos;
    vec3 power;
    f32 phi;
    f32 theta;

    Photon(vec3 pos, vec3 power, f32 phi, f32 theta) : pos(pos), power(power), phi(phi), theta(theta) {}
};

#endif // PHOSPHOR_PHOTON_HPP
