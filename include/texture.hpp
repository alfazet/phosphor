#ifndef PHOSPHOR_TEXTURE_HPP
#define PHOSPHOR_TEXTURE_HPP

#include "stb_image.h"
#include "typedefs.h"

#include <string>
#include <vector>

enum TextureChannel { CHANNEL_R, CHANNEL_G, CHANNEL_B };

struct UvTransform {
    float2 offset;
};

struct Texture {
    u32 width;
    u32 height;
    u32 channels;
    std::string name;
    std::vector<u8> tex_atlas; // all mipmap levels packed
    std::vector<u32> tex_offsets;
    std::vector<u32> tex_widths;
    std::vector<u32> tex_heights;
};

#endif // PHOSPHOR_TEXTURE_HPP
