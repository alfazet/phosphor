#ifndef PHOSPHOR_PHOTON_HPP
#define PHOSPHOR_PHOTON_HPP

#include "typedefs.h"

struct Photon {
    float4 pos;
    float4 power;
    f32 phi;
    f32 theta;
    u8 plane;

    Photon(float4 pos, float4 power, f32 phi, f32 theta, u8 plane)
        : pos(pos), power(power), phi(phi), theta(theta), plane(plane) {}
};

#endif // PHOSPHOR_PHOTON_HPP
