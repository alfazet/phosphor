#ifndef PHOSPHOR_PHOTON_H
#define PHOSPHOR_PHOTON_H

typedef struct Photon {
    float4 pos;
    float4 power;
    float4 dir; // incoming direction normalized

    // total: 48
    // u8 _padding[0];
} Photon __attribute__((aligned(16)));

#endif // PHOSPHOR_PHOTON_H
