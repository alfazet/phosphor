#ifndef PHOSPHOR_TEXTURE_META_H
#define PHOSPHOR_TEXTURE_META_H

#include "typedefs.h"

typedef struct TextureMeta {
    float2 uv_offset;
    float2 uv_scale;
    f32 uv_rotation;
    // 2 * 2 * 4 + 4 = 20

    u32 atlas_offset; // offset into the atlas for mip level 0
    u32 width;
    u32 height;
    u32 channels;
    // 4 * 4 = 16

    u32 mip_levels_count;
    // offset into tex_mip_offsets and into tex_mip_dims
    // to access pixel (`u`, `v`) of this texture at mip level `k`:
    // - atlas_offset = tex_mip_offsets[mip_table_offset + k]
    // - width = tex_mip_dims[2 * (mip_table_offset + k)]
    // - pixel_{r,g,b} = tex_atlas[atlas_offset + v * width + u + {0,1,2}]
    u32 mip_table_offset;
    // 2 * 4 = 8

    // total: 44
    u8 _padding[4];
} TextureMeta __attribute__((aligned(16)));

#endif // PHOSPHOR_TEXTURE_META_H
