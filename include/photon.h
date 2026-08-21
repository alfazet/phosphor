#ifndef PHOSPHOR_PHOTON_H
#define PHOSPHOR_PHOTON_H

typedef struct Photon {
    float4 pos;
    float4 power;
    float4 dir;    // incoming direction normalized
    float4 normal; // normal of the surface hit
    u32 axis;      // for kd-tree

    // total: 68
    u8 _padding[12];
} Photon __attribute__((aligned(16)));

#endif // PHOSPHOR_PHOTON_H
