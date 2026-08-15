#ifndef PHOSPHOR_MOCK_SCENE_H
#define PHOSPHOR_MOCK_SCENE_H

#include "typedefs.h"

bool intersect_scene(float4 origin, float4 dir, f32 *t, float4 *normal, usize *mat_id) {
    // hardcoded ground plane at y = 0
    if (dir.y < 0.0f) {
        *t = -origin.y / dir.y;
        *normal = (float4){0.0f, 1.0f, 0.0f, 0.0f};
        *mat_id = 0;
        return *t > 0.0f;
    }
    return false;
}

#endif // PHOSPHOR_MOCK_SCENE_H
