#ifndef PHOSPHOR_TEXTURE_META_H
#define PHOSPHOR_TEXTURE_META_H

#include "constants.h"
#include "typedefs.h"

typedef struct GPU_ALIGN TextureMeta {
    u32 atlas_offset; // offset into the atlas for mip level 0
    u32 width;
    u32 height;
    u32 channels;
    // 4 * 4 = 16

    // total: 16
    u8 _padding[16];
} TextureMeta;

#endif // PHOSPHOR_TEXTURE_META_H
