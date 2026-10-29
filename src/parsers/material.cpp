#include "host/parsers/material.hpp"
#include "common/constants.h"
#include "host/glm_bundle.hpp"
#include "host/parsers/texture.hpp"

#include <assimp/material.h>
#include <filesystem>

static Material make_default_material() {
    Material mat{};
    mat.base_color = float4{{1.0f, 1.0f, 1.0f, 1.0f}};
    mat.emissive = float4{{0.0f, 0.0f, 0.0f, 0.0f}};
    mat.metallic = 0.0f;
    mat.roughness = 1.0f;
    mat.transmission = DEFAULT_TRANSMISSION;
    mat.ior = DEFAULT_IOR;

    mat.diff_index = NO_TEXTURE;
    mat.emis_index = NO_TEXTURE;
    mat.norm_index = NO_TEXTURE;
    mat.occlusion_index = NO_TEXTURE;
    mat.metal_rough_index = NO_TEXTURE;
    mat.transmission_index = NO_TEXTURE;

    mat.att_color = float4{{1.0f, 1.0f, 1.0f, 0.0f}};
    mat.att_dist = INF;
    mat.thickness = 0.0f;

    return mat;
}

static UvTransformBlock make_default_uv_transforms() {
    UvTransform identity{};
    identity.uv_offset = float2{{0.0f, 0.0f}};
    identity.uv_scale = float2{{1.0f, 1.0f}};
    identity.uv_rotation = 0.0f;

    UvTransformBlock block{};
    block.diff = identity;
    block.emis = identity;
    block.norm = identity;
    block.occlusion = identity;
    block.metal_rough = identity;
    block.transmission = identity;

    return block;
}

static void parse_pbr_metallic_roughness(aiMaterial *ai_mat, const SceneData &scene, Material &out,
                                         UvTransformBlock &out_uv) {
    const aiTextureType uv_types[] = {
        aiTextureType_DIFFUSE,           aiTextureType_BASE_COLOR,   aiTextureType_NORMALS,
        aiTextureType_EMISSIVE,          aiTextureType_METALNESS,    aiTextureType_DIFFUSE_ROUGHNESS,
        aiTextureType_AMBIENT_OCCLUSION, aiTextureType_TRANSMISSION, aiTextureType_UNKNOWN};
    for (auto type : uv_types) {
        aiUVTransform ai_uv{};
        UvTransform *transform;
        if (ai_mat->Get(AI_MATKEY_UVTRANSFORM(type, 0), ai_uv) == AI_SUCCESS) {
            switch (type) {
            case aiTextureType_DIFFUSE:
                transform = &out_uv.diff;
                break;
            case aiTextureType_BASE_COLOR:
                transform = &out_uv.diff;
                break;
            case aiTextureType_NORMALS:
                transform = &out_uv.norm;
                break;
            case aiTextureType_EMISSIVE:
                transform = &out_uv.emis;
                break;
            case aiTextureType_METALNESS:
                transform = &out_uv.metal_rough;
                break;
            case aiTextureType_DIFFUSE_ROUGHNESS:
                transform = &out_uv.diff;
                break;
            case aiTextureType_AMBIENT_OCCLUSION:
                transform = &out_uv.occlusion;
                break;
            case aiTextureType_TRANSMISSION:
                transform = &out_uv.transmission;
                break;
            default:
                transform = &out_uv.diff;
                break;
            }
            transform->uv_offset = float2{{ai_uv.mTranslation.x, ai_uv.mTranslation.y}};
            transform->uv_scale = float2{{ai_uv.mScaling.x, ai_uv.mScaling.y}};
            transform->uv_rotation = ai_uv.mRotation;
        }
    }

    aiColor4D base_color(1.0f, 1.0f, 1.0f, 1.0f);
    if (ai_mat->Get(AI_MATKEY_BASE_COLOR, base_color) == AI_SUCCESS)
        out.base_color = float4{{base_color.r, base_color.g, base_color.b, base_color.a}};

    ai_mat->Get(AI_MATKEY_METALLIC_FACTOR, out.metallic);
    ai_mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, out.roughness);

    aiColor3D emissive(0.0f, 0.0f, 0.0f);
    ai_mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
    f32 emissive_intensity = 1.0f;
    ai_mat->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissive_intensity);
    vec3 emissive_vec = vec3(emissive.r, emissive.g, emissive.b) * emissive_intensity;
    out.emissive = vec3_to_float4(emissive_vec);
}

static void parse_khr_transmission(aiMaterial *ai_mat, const SceneData &scene, Material &out) {
    ai_mat->Get(AI_MATKEY_TRANSMISSION_FACTOR, out.transmission);

    aiString path;
    if (ai_mat->GetTexture(AI_MATKEY_TRANSMISSION_TEXTURE, &path) == AI_SUCCESS) {
        std::string name = std::filesystem::path(path.C_Str()).filename().string();
        if (auto idx = find_texture(name, scene.textures))
            out.transmission_index = *idx;
    }
}

static void parse_khr_ior(aiMaterial *ai_mat, Material &out) { ai_mat->Get(AI_MATKEY_REFRACTI, out.ior); }

static void parse_khr_volume(aiMaterial *ai_mat, Material &out) {
    ai_mat->Get(AI_MATKEY_VOLUME_THICKNESS_FACTOR, out.thickness);
    ai_mat->Get(AI_MATKEY_VOLUME_ATTENUATION_DISTANCE, out.att_dist);

    aiColor3D att_color(1.0f, 1.0f, 1.0f);
    if (ai_mat->Get(AI_MATKEY_VOLUME_ATTENUATION_COLOR, att_color) == AI_SUCCESS)
        out.att_color = float4{{att_color.r, att_color.g, att_color.b, 1.0f}};
}

static void parse_material_textures(aiMaterial *ai_mat, const SceneData &scene, Material &out) {
    aiString path;

    auto try_set = [&](aiTextureType type, u32 &index_field) {
        if (ai_mat->GetTexture(type, 0, &path) == AI_SUCCESS) {
            std::string name = std::filesystem::path(path.C_Str()).filename().string();
            if (auto idx = find_texture(name, scene.textures))
                index_field = *idx;
        }
    };

    try_set(aiTextureType_DIFFUSE, out.diff_index);
    if (out.diff_index == NO_TEXTURE)
        try_set(aiTextureType_BASE_COLOR, out.diff_index);

    try_set(aiTextureType_EMISSIVE, out.emis_index);
    try_set(aiTextureType_NORMALS, out.norm_index);
    try_set(aiTextureType_AMBIENT_OCCLUSION, out.occlusion_index);
    if (ai_mat->GetTexture(aiTextureType_METALNESS, 0, &path) == AI_SUCCESS ||
        ai_mat->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &path) == AI_SUCCESS) {
        std::string name = std::filesystem::path(path.C_Str()).filename().string();
        if (auto idx = find_texture(name, scene.textures))
            out.metal_rough_index = *idx;
    }
}

u32 parse_material(const aiScene *scene, aiMesh *mesh, SceneData &out_scene, std::optional<vec3> &emissive_out) {
    Material mat = make_default_material();
    UvTransformBlock uv = make_default_uv_transforms();
    emissive_out = std::nullopt;
    if (mesh->mMaterialIndex >= scene->mNumMaterials) {
        u32 idx = out_scene.materials.size();
        out_scene.materials.push_back(mat);
        out_scene.uv_transforms.push_back(uv);
        return idx;
    }

    aiMaterial *ai_mat = scene->mMaterials[mesh->mMaterialIndex];

    parse_pbr_metallic_roughness(ai_mat, out_scene, mat, uv);
    parse_khr_transmission(ai_mat, out_scene, mat);
    parse_khr_ior(ai_mat, mat);
    parse_khr_volume(ai_mat, mat);
    parse_material_textures(ai_mat, out_scene, mat);

    vec3 emissive_vec(mat.emissive.x, mat.emissive.y, mat.emissive.z);
    if (glm::any(glm::greaterThan(emissive_vec, vec3(EPS))))
        emissive_out = emissive_vec;
    u32 idx = out_scene.materials.size();
    out_scene.materials.push_back(mat);
    out_scene.uv_transforms.push_back(uv);

    return idx;
}
