#ifndef PHOSPHOR_RAY_H
#define PHOSPHOR_RAY_H

#include "types.h"

struct Ray {
    float4 origin;
    float4 dir;
    float4 dp_dx, dd_dx, dp_dy, dd_dy;
};

#endif // PHOSPHOR_RAY_H
