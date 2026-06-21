#ifndef PHOSPHOR_POINTLIGHT_HPP
#define PHOSPHOR_POINTLIGHT_HPP

#include "common.hpp"
#include "ray.hpp"
#include <vector>

struct Pointlight {
    vec3 pos;
    f32 power;
};

#endif // PHOSPHOR_POINTLIGHT_HPP