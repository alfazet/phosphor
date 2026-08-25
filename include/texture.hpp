#ifndef PHOSPHOR_TEXTURE_HPP
#define PHOSPHOR_TEXTURE_HPP

#include "typedefs.h"

#include <assimp/material.h>
#include <assimp/scene.h>

#include <optional>
#include <string>
#include <vector>

enum TextureChannel { CHANNEL_R, CHANNEL_G, CHANNEL_B };

struct UvTransform {
    float2 offset;
};

struct Texture {
    u32 width = 0;
    u32 height = 0;
    u32 channels = 3;
    std::string name;
    std::vector<u8> tex_atlas; // all mipmap levels packed
    std::vector<u32> tex_offsets;
    std::vector<u32> tex_widths;
    std::vector<u32> tex_heights;
};

struct SceneData;

std::optional<u32> find_texture(const std::string &name, const std::vector<Texture> &textures);

void build_mip_chain(Texture &tex, std::vector<u8> pixels, u32 w, u32 h);

void load_texture(const aiScene *scene, aiMaterial *mat, aiTextureType type, const char *dir, SceneData &out_scene);

void parse_textures(const aiScene *scene, SceneData &out_scene, const char *dir);

#endif // PHOSPHOR_TEXTURE_HPP
