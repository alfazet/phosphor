#include "texture.hpp"
#include "logger.hpp"
#include "scene.hpp"
#include "stb_image.h"

#include <assimp/material.h>
#include <assimp/scene.h>
#include <filesystem>

std::optional<u32> find_texture(const std::string &name, const std::vector<Texture> &textures) {
    for (u32 i = 0; i < textures.size(); i++) {
        if (textures[i].name == name)
            return i;
    }
    return std::nullopt;
}

void build_mip_chain(Texture &tex, std::vector<u8> pixels, u32 w, u32 h) {
    tex.width = w;
    tex.height = h;
    tex.channels = 3;

    std::vector<u8> level = std::move(pixels);
    u32 lw = w, lh = h;
    while (true) {
        tex.tex_offsets.push_back(static_cast<u32>(tex.tex_atlas.size()));
        tex.tex_widths.push_back(lw);
        tex.tex_heights.push_back(lh);
        tex.tex_atlas.insert(tex.tex_atlas.end(), level.begin(), level.end());
        if (lw == 1 && lh == 1)
            break;

        u32 nw = std::max(1u, lw / 2);
        u32 nh = std::max(1u, lh / 2);
        std::vector<u8> next(nw * nh * tex.channels);
        for (u32 y = 0; y < nh; y++) {
            for (u32 x = 0; x < nw; x++) {
                u32 sx0 = std::min(x * 2, lw - 1);
                u32 sy0 = std::min(y * 2, lh - 1);
                u32 sx1 = std::min(x * 2 + 1, lw - 1);
                u32 sy1 = std::min(y * 2 + 1, lh - 1);
                for (u32 c = 0; c < 3; c++) {
                    u32 sum = level[(sy0 * lw + sx0) * tex.channels + c] + level[(sy0 * lw + sx1) * tex.channels + c] +
                              level[(sy1 * lw + sx0) * tex.channels + c] + level[(sy1 * lw + sx1) * tex.channels + c];
                    next[(y * nw + x) * tex.channels + c] = static_cast<u8>(sum / (tex.channels + 1));
                }
            }
        }
        level = std::move(next);
        lw = nw;
        lh = nh;
    }
}

void load_texture(const aiScene *aiscene, aiMaterial *mat, aiTextureType type, const char *directory,
                  SceneData &out_scene) {
    aiString path;
    if (mat->GetTexture(type, 0, &path) != AI_SUCCESS)
        return;

    std::string name = std::filesystem::path(path.C_Str()).filename().string();
    if (find_texture(name, out_scene.textures).has_value())
        return;

    LOG_INFO("loading texture from {} of type {}", path.C_Str(), aiTextureTypeToString(type));

    Texture t;
    t.name = name;
    const aiTexture *embedded_tex = aiscene->GetEmbeddedTexture(path.C_Str());
    i32 w = 0, h = 0, c = 0;
    u8 *raw = nullptr;
    std::vector<u8> pixels;

    if (embedded_tex) {
        if (embedded_tex->mHeight == 0) {
            // compressed
            const u8 *buf = reinterpret_cast<const u8 *>(embedded_tex->pcData);
            raw = stbi_load_from_memory(buf, static_cast<i32>(embedded_tex->mWidth), &w, &h, &c, 3);
            if (!raw) {
                LOG_ERROR("failed to decode embedded texture {}", path.C_Str());
                return;
            }
            pixels.assign(raw, raw + (static_cast<u32>(w) * h * 3));
            stbi_image_free(raw);
        } else {
            // uncompressed ARGB8888
            w = static_cast<i32>(embedded_tex->mWidth);
            h = static_cast<i32>(embedded_tex->mHeight);
            pixels.resize(static_cast<u32>(w) * h * 3);
            const aiTexel *texels = embedded_tex->pcData;
            for (i32 i = 0; i < w * h; ++i) {
                pixels[i * 3 + 0] = texels[i].r;
                pixels[i * 3 + 1] = texels[i].g;
                pixels[i * 3 + 2] = texels[i].b;
            }
        }
    } else {
        std::string full_path = std::string(directory) + "/" + path.C_Str();
        raw = stbi_load(full_path.c_str(), &w, &h, &c, 3);
        if (!raw) {
            LOG_ERROR("failed to load texture from {}", full_path);
            return;
        }
        pixels.assign(raw, raw + (static_cast<u32>(w) * h * 3));
        stbi_image_free(raw);
    }

    build_mip_chain(t, std::move(pixels), static_cast<u32>(w), static_cast<u32>(h));
    out_scene.textures.push_back(std::move(t));
}

void parse_textures(const aiScene *aiscene, SceneData &out_scene, const char *directory) {
    for (u32 i = 0; i < aiscene->mNumMaterials; i++) {
        aiMaterial *mat = aiscene->mMaterials[i];
        load_texture(aiscene, mat, aiTextureType_DIFFUSE, directory, out_scene);
        load_texture(aiscene, mat, aiTextureType_BASE_COLOR, directory, out_scene);
        load_texture(aiscene, mat, aiTextureType_EMISSIVE, directory, out_scene);
        load_texture(aiscene, mat, aiTextureType_NORMALS, directory, out_scene);
        load_texture(aiscene, mat, aiTextureType_AMBIENT_OCCLUSION, directory, out_scene);
        load_texture(aiscene, mat, aiTextureType_METALNESS, directory, out_scene);
        load_texture(aiscene, mat, aiTextureType_DIFFUSE_ROUGHNESS, directory, out_scene);
        load_texture(aiscene, mat, aiTextureType_TRANSMISSION, directory, out_scene);
    }
}
