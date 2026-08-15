#include "scene.hpp"
#include "constants.h"
#include "logger.hpp"
#include "stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cstring>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

using glm::mat3;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

inline float4 to_float4(const vec3 &v, f32 w = 0.0f) { return float4{{v.x, v.y, v.z, w}}; }
inline float2 to_float2(const vec2 &v) { return float2{{v.x, v.y}}; }

inline f32 bits_as_float(u32 x) {
    f32 f;
    std::memcpy(&f, &x, sizeof(f));
    return f;
}

mat4 ai_matrix4x4_to_glm(const aiMatrix4x4 &from) {
    mat4 to;
    to[0][0] = from.a1;
    to[1][0] = from.a2;
    to[2][0] = from.a3;
    to[3][0] = from.a4;
    to[0][1] = from.b1;
    to[1][1] = from.b2;
    to[2][1] = from.b3;
    to[3][1] = from.b4;
    to[0][2] = from.c1;
    to[1][2] = from.c2;
    to[2][2] = from.c3;
    to[3][2] = from.c4;
    to[0][3] = from.d1;
    to[1][3] = from.d2;
    to[2][3] = from.d3;
    to[3][3] = from.d4;
    return to;
}

Camera build_camera(vec3 position, vec3 look_at, vec3 up, f32 hfov_deg, f32 aspect) {
    Camera c{};

    vec3 w_dir = glm::normalize(position - look_at);
    vec3 u_dir = glm::normalize(glm::cross(up, w_dir));
    vec3 v_dir = glm::cross(w_dir, u_dir);

    f32 hfov_rad = glm::radians(hfov_deg);
    f32 half_width = std::tan(hfov_rad * 0.5f);
    f32 half_height = half_width / aspect;

    vec3 horizontal = 2.0f * half_width * u_dir;
    vec3 vertical = 2.0f * half_height * v_dir;
    vec3 lower_left = position - 0.5f * horizontal - 0.5f * vertical - w_dir;

    c.position = to_float4(position);
    c.target = to_float4(look_at);
    c.up = to_float4(up);
    c.lower_left_corner = to_float4(lower_left);
    c.horizontal = to_float4(horizontal);
    c.vertical = to_float4(vertical);
    c.u = to_float4(u_dir);
    c.v = to_float4(v_dir);
    c.w = to_float4(w_dir);
    c.hfov = hfov_deg;
    c.aspect_ratio = aspect;
    return c;
}

void parse_camera(const aiCamera *ai_camera, SceneData &out_scene, mat4 global_transform) {
    vec3 position =
        vec3(global_transform * vec4(ai_camera->mPosition.x, ai_camera->mPosition.y, ai_camera->mPosition.z, 1.0f));
    vec3 look_dir =
        vec3(global_transform * vec4(ai_camera->mLookAt.x, ai_camera->mLookAt.y, ai_camera->mLookAt.z, 0.0f));
    // http://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices/ why 0.0f 1.0f
    vec3 look_at_world = position + look_dir;

    vec3 up = glm::normalize(vec3(global_transform * vec4(ai_camera->mUp.x, ai_camera->mUp.y, ai_camera->mUp.z, 0.0f)));

    ASSERT(ai_camera->mHorizontalFOV > 0 && ai_camera->mHorizontalFOV < 180,
           "hfov must be between 0 and 180 degrees exclusive");
    ASSERT(ai_camera->mAspect > 0, "aspect_ratio must be positive");
    f32 fov_h_deg = ai_camera->mHorizontalFOV * (180.0f / static_cast<f32>(M_PI));
    f32 aspect = ai_camera->mAspect > 0.0f ? ai_camera->mAspect : (16.0f / 9.0f);

    out_scene.cameras.push_back(build_camera(position, look_at_world, up, fov_h_deg, aspect));
}

Light make_point_light(vec3 position, vec3 power) {
    Light l{};
    l.kind = LIGHT_POINT;
    l.position = to_float4(position);
    l.power = to_float4(power);
    return l;
}

Light make_spot_light(vec3 position, vec3 power, vec3 direction, f32 inner_rad, f32 outer_rad) {
    Light l{};
    l.kind = LIGHT_SPOT;
    l.position = to_float4(position);
    l.power = to_float4(power);
    l.direction = to_float4(direction);
    l.aux = float4{{inner_rad, outer_rad, 0.0f, 0.0f}};
    return l;
}

Light make_directional_light(vec3 direction, vec3 power, f32 radius = 0.0f) {
    Light l{};
    l.kind = LIGHT_DIRECTIONAL;
    l.direction = to_float4(direction);
    l.power = to_float4(power);
    l.aux = float4{{radius, 0.0f, 0.0f, 0.0f}};
    return l;
}

Light make_textured_light(u32 tex_index, u32 tri_start, u32 tri_count) {
    Light l{};
    l.kind = LIGHT_TEXTURED;
    l.aux = float4{{bits_as_float(tex_index), bits_as_float(tri_start), bits_as_float(tri_count), 0.0f}};
    return l;
}

void parse_light(const aiLight *ai_light, SceneData &out_scene, mat4 global_transform) {
    vec3 position =
        vec3(global_transform * vec4(ai_light->mPosition.x, ai_light->mPosition.y, ai_light->mPosition.z, 1.0f));
    mat3 global_rotation(global_transform);
    // diffuse/specular values are pre-multiplied by light intensity I (in candelas) taken from the gltf file
    // assuming a light with power P (in watts, as specified in Blender), and luminous efficacy K = 683 cd * sr / W,
    // we have I = (P * K) / (4 * pi) = 54.35 * P
    // so, for example, a 1000 W pure red light will be represented as approx. (54350, 0, 0)

    // mColorDiffuse is the same as mColorSpecular in gltf
    vec3 power =
        vec3(ai_light->mColorDiffuse.r, ai_light->mColorDiffuse.g, ai_light->mColorDiffuse.b) / LUMINOUS_EFFICACY;

    if (ai_light->mType == aiLightSource_POINT) {
        out_scene.lights.push_back(make_point_light(position, power));
    } else if (ai_light->mType == aiLightSource_SPOT) {
        f32 inner = ai_light->mAngleInnerCone;
        f32 outer = ai_light->mAngleOuterCone;
        vec3 dir = glm::normalize(global_rotation *
                                  vec3(ai_light->mDirection.x, ai_light->mDirection.y, ai_light->mDirection.z));
        out_scene.lights.push_back(make_spot_light(position, power, dir, inner, outer));
    } else if (ai_light->mType == aiLightSource_DIRECTIONAL) {
        vec3 dir = glm::normalize(global_rotation *
                                  vec3(ai_light->mDirection.x, ai_light->mDirection.y, ai_light->mDirection.z));
        out_scene.lights.push_back(make_directional_light(dir, power * LUMINOUS_EFFICACY));
    } else {
        LOG_WARN("scene contains an unsupported light type: {}", static_cast<i32>(ai_light->mType));
    }
}

std::optional<u32> find_texture(const std::string &name, const std::vector<Texture> &textures) {
    for (u32 i = 0; i < textures.size(); i++) {
        if (textures[i].name == name)
            return i;
    }
    return std::nullopt;
}

// TODO look at this
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
        std::vector<u8> next(static_cast<usize>(nw) * nh * 3);
        for (u32 y = 0; y < nh; y++) {
            for (u32 x = 0; x < nw; x++) {
                u32 sx0 = std::min(x * 2, lw - 1);
                u32 sy0 = std::min(y * 2, lh - 1);
                u32 sx1 = std::min(x * 2 + 1, lw - 1);
                u32 sy1 = std::min(y * 2 + 1, lh - 1);
                for (u32 c = 0; c < 3; c++) {
                    u32 sum = level[(sy0 * lw + sx0) * 3 + c] + level[(sy0 * lw + sx1) * 3 + c] +
                              level[(sy1 * lw + sx0) * 3 + c] + level[(sy1 * lw + sx1) * 3 + c];
                    next[(y * nw + x) * 3 + c] = static_cast<u8>(sum / 4);
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

    // load embedded
    if (embedded_tex) {
        if (embedded_tex->mHeight == 0) {
            // compressed
            const u8 *buf = reinterpret_cast<const u8 *>(embedded_tex->pcData);
            raw = stbi_load_from_memory(buf, static_cast<i32>(embedded_tex->mWidth), &w, &h, &c, 3);
            if (!raw) {
                LOG_ERROR("failed to decode embedded texture {}", path.C_Str());
                return;
            }
            pixels.assign(raw, raw + (static_cast<usize>(w) * h * 3));
            stbi_image_free(raw);
        } else {
            // uncompressed
            w = static_cast<i32>(embedded_tex->mWidth);
            h = static_cast<i32>(embedded_tex->mHeight);
            pixels.resize(static_cast<usize>(w) * h * 3);
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
        pixels.assign(raw, raw + (static_cast<usize>(w) * h * 3));
        stbi_image_free(raw);
    }

    build_mip_chain(t, pixels, static_cast<u32>(w), static_cast<u32>(h));
    out_scene.textures.push_back(std::move(t));
}

void parse_textures(const aiScene *aiscene, SceneData &out_scene, const char *directory) {
    for (u32 i = 0; i < aiscene->mNumMaterials; i++) {
        aiMaterial *mat = aiscene->mMaterials[i];
        load_texture(aiscene, mat, aiTextureType_DIFFUSE, directory, out_scene);
        load_texture(aiscene, mat, aiTextureType_EMISSIVE, directory, out_scene);
        load_texture(aiscene, mat, aiTextureType_NORMALS, directory, out_scene);
        load_texture(aiscene, mat, aiTextureType_AMBIENT_OCCLUSION, directory, out_scene);
        load_texture(aiscene, mat, aiTextureType_METALNESS, directory, out_scene);
        load_texture(aiscene, mat, aiTextureType_DIFFUSE_ROUGHNESS, directory, out_scene);
    }
}

u32 parse_material(const aiScene *scene, aiMesh *mesh, SceneData &out_scene, std::optional<vec3> &emissive_out) {
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
    mat.uv_offset = float2{{0.0f, 0.0f}};
    mat.uv_scale = float2{{1.0f, 1.0f}};
    mat.uv_rotation = 0.0f;

    emissive_out = std::nullopt;
    if (mesh->mMaterialIndex >= scene->mNumMaterials) {
        u32 idx = static_cast<u32>(out_scene.materials.size());
        out_scene.materials.push_back(mat);
        return idx;
    }

    aiMaterial *ai_mat = scene->mMaterials[mesh->mMaterialIndex];
    aiUVTransform ai_uv_transform;
    if (ai_mat->Get(AI_MATKEY_UVTRANSFORM(aiTextureType_DIFFUSE, 0), ai_uv_transform) == AI_SUCCESS) {
        mat.uv_offset = float2{{ai_uv_transform.mTranslation.x, ai_uv_transform.mTranslation.y}};
        mat.uv_scale = float2{{ai_uv_transform.mScaling.x, ai_uv_transform.mScaling.y}};
        mat.uv_rotation = ai_uv_transform.mRotation;
    }

    aiColor4D base_color(1.0f, 1.0f, 1.0f, 1.0f);
    if (ai_mat->Get(AI_MATKEY_BASE_COLOR, base_color) == AI_SUCCESS)
        mat.base_color = float4{{base_color.r, base_color.g, base_color.b, base_color.a}};

    ai_mat->Get(AI_MATKEY_METALLIC_FACTOR, mat.metallic);
    ai_mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, mat.roughness);
    ai_mat->Get(AI_MATKEY_TRANSMISSION_FACTOR, mat.transmission);
    ai_mat->Get(AI_MATKEY_REFRACTI, mat.ior);

    aiColor3D emissive(0.0f, 0.0f, 0.0f);
    ai_mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive);
    f32 emissive_intensity = 1.0f;
    ai_mat->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissive_intensity);
    vec3 emissive_vec = vec3(emissive.r, emissive.g, emissive.b) * emissive_intensity;
    mat.emissive = to_float4(emissive_vec);
    if (glm::any(glm::greaterThan(emissive_vec, vec3(EPS))))
        emissive_out = emissive_vec;

    aiString path;
    if (ai_mat->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
        std::string name = std::filesystem::path(path.C_Str()).filename().string();
        if (auto idx = find_texture(name, out_scene.textures))
            mat.diff_index = *idx;
    }
    if (ai_mat->GetTexture(aiTextureType_EMISSIVE, 0, &path) == AI_SUCCESS) {
        std::string name = std::filesystem::path(path.C_Str()).filename().string();
        if (auto idx = find_texture(name, out_scene.textures))
            mat.emis_index = *idx;
    }
    if (ai_mat->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS) {
        std::string name = std::filesystem::path(path.C_Str()).filename().string();
        if (auto idx = find_texture(name, out_scene.textures))
            mat.norm_index = *idx;
    }
    if (ai_mat->GetTexture(aiTextureType_METALNESS, 0, &path) == AI_SUCCESS ||
        ai_mat->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &path) == AI_SUCCESS) {
        std::string name = std::filesystem::path(path.C_Str()).filename().string();
        if (auto idx = find_texture(name, out_scene.textures))
            mat.metal_rough_index = *idx;
    }
    if (ai_mat->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &path) == AI_SUCCESS) {
        std::string name = std::filesystem::path(path.C_Str()).filename().string();
        if (auto idx = find_texture(name, out_scene.textures))
            mat.occlusion_index = *idx;
    }

    u32 idx = static_cast<u32>(out_scene.materials.size());
    out_scene.materials.push_back(mat);
    return idx;
}

void process_node(aiNode *node, const aiScene *aiscene, SceneData &out_scene, mat4 parent_transform) {
    mat4 local_transform = ai_matrix4x4_to_glm(node->mTransformation);
    mat4 global_transform = parent_transform * local_transform;

    // check if node is a camera
    for (u32 i = 0; i < aiscene->mNumCameras; i++) {
        aiCamera *ai_cam = aiscene->mCameras[i];
        if (ai_cam->mName == node->mName)
            parse_camera(ai_cam, out_scene, global_transform);
    }

    // check if node is a light
    for (u32 i = 0; i < aiscene->mNumLights; i++) {
        aiLight *ai_light = aiscene->mLights[i];
        if (ai_light->mName == node->mName)
            parse_light(ai_light, out_scene, global_transform);
    }

    // handle meshes
    for (usize i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = aiscene->mMeshes[node->mMeshes[i]];
        mat3 normal_matrix = glm::transpose(glm::inverse(mat3(global_transform)));

        std::optional<vec3> emissive;
        u32 mat_index = parse_material(aiscene, mesh, out_scene, emissive);

        u32 triangle_start = static_cast<u32>(out_scene.triangles.size());
        for (usize f = 0; f < mesh->mNumFaces; f++) {
            aiFace face = mesh->mFaces[f];
            aiVector3D v0 = mesh->mVertices[face.mIndices[0]];
            aiVector3D v1 = mesh->mVertices[face.mIndices[1]];
            aiVector3D v2 = mesh->mVertices[face.mIndices[2]];
            vec3 p0 = vec3(global_transform * vec4(v0.x, v0.y, v0.z, 1.0f));
            vec3 p1 = vec3(global_transform * vec4(v1.x, v1.y, v1.z, 1.0f));
            vec3 p2 = vec3(global_transform * vec4(v2.x, v2.y, v2.z, 1.0f));

            vec2 uv0(0.0f), uv1(0.0f), uv2(0.0f);
            if (mesh->mTextureCoords[0]) {
                aiVector3D t0 = mesh->mTextureCoords[0][face.mIndices[0]];
                aiVector3D t1 = mesh->mTextureCoords[0][face.mIndices[1]];
                aiVector3D t2 = mesh->mTextureCoords[0][face.mIndices[2]];
                uv0 = vec2(t0.x, t0.y);
                uv1 = vec2(t1.x, t1.y);
                uv2 = vec2(t2.x, t2.y);
            }

            vec3 n0, n1, n2;
            if (mesh->HasNormals()) {
                aiVector3D an0 = mesh->mNormals[face.mIndices[0]];
                aiVector3D an1 = mesh->mNormals[face.mIndices[1]];
                aiVector3D an2 = mesh->mNormals[face.mIndices[2]];

                n0 = glm::normalize(normal_matrix * vec3(an0.x, an0.y, an0.z));
                n1 = glm::normalize(normal_matrix * vec3(an1.x, an1.y, an1.z));
                n2 = glm::normalize(normal_matrix * vec3(an2.x, an2.y, an2.z));
            } else {
                vec3 face_normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
                n0 = n1 = n2 = face_normal;
            }

            vec3 t0, t1, t2;
            if (mesh->mTangents) {
                aiVector3D at0 = mesh->mTangents[face.mIndices[0]];
                aiVector3D at1 = mesh->mTangents[face.mIndices[1]];
                aiVector3D at2 = mesh->mTangents[face.mIndices[2]];

                t0 = glm::normalize(normal_matrix * vec3(at0.x, at0.y, at0.z));
                t1 = glm::normalize(normal_matrix * vec3(at1.x, at1.y, at1.z));
                t2 = glm::normalize(normal_matrix * vec3(at2.x, at2.y, at2.z));
            } else {
                vec3 edge1 = p1 - p0;
                vec3 edge2 = p2 - p0;
                vec2 duv1 = uv1 - uv0;
                vec2 duv2 = uv2 - uv0;
                f32 det = duv1.x * duv2.y - duv2.x * duv1.y;
                vec3 face_tangent;
                if (glm::abs(det) < EPS) {
                    face_tangent = vec3(1, 0, 0);
                } else {
                    f32 inv_det = 1.0f / det;
                    face_tangent = inv_det * (duv2.y * edge1 - duv1.y * edge2);
                }
                face_tangent = glm::normalize(normal_matrix * face_tangent);
                t0 = t1 = t2 = face_tangent;
            }

            HostTriangle tri{};
            tri.v0 = to_float4(p0);
            tri.v1 = to_float4(p1);
            tri.v2 = to_float4(p2);
            tri.n0 = to_float4(n0);
            tri.n1 = to_float4(n1);
            tri.n2 = to_float4(n2);
            tri.t0 = to_float4(t0);
            tri.t1 = to_float4(t1);
            tri.t2 = to_float4(t2);
            tri.uv0 = to_float2(uv0);
            tri.uv1 = to_float2(uv1);
            tri.uv2 = to_float2(uv2);
            tri.mat_index = mat_index;
            out_scene.triangles.push_back(tri);
        }

        u32 triangle_count = static_cast<u32>(out_scene.triangles.size()) - triangle_start;
        if (emissive.has_value() && triangle_count > 0) {
            const Material &m = out_scene.materials[mat_index];
            out_scene.lights.push_back(make_textured_light(m.emis_index, triangle_start, triangle_count));
        }
    }

    for (usize i = 0; i < node->mNumChildren; i++) {
        process_node(node->mChildren[i], aiscene, out_scene, global_transform);
    }
}

void build_light_pref_sum(SceneData &out_scene) {
    out_scene.light_area_pref_sum.clear();
    out_scene.light_area_pref_sum.reserve(out_scene.lights.size());
    f32 running = 0.0f;
    for (const auto &l : out_scene.lights) {
        f32 luminance = l.power.x * 0.2126f + l.power.y * 0.7152f + l.power.z * 0.0722f;
        running += std::max(luminance, 0.0f);
        out_scene.light_area_pref_sum.push_back(running);
    }
}

void add_default_camera(SceneData &out_scene) {
    out_scene.cameras.push_back(
        build_camera(vec3(0.0f, 0.0f, 3.0f), vec3(0.0f), vec3(0.0f, 1.0f, 0.0f), 60.0f, 16.0f / 9.0f));
}

SceneData read_file(const char *file_name) {
    TimerScope timer_scope("loading scene");

    SceneData scene{};
    Assimp::Importer importer;
    const aiScene *aiscene = importer.ReadFile(file_name, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                                                              aiProcess_CalcTangentSpace);

    if (!aiscene || aiscene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiscene->mRootNode) {
        LOG_ERROR("while parsing scene {}: {}", file_name, importer.GetErrorString());
        return scene;
    }

    std::filesystem::path p(file_name);
    std::string texture_dir = p.parent_path().string();
    parse_textures(aiscene, scene, texture_dir.c_str());

    mat4 identity(1.0f);
    process_node(aiscene->mRootNode, aiscene, scene, identity);

    build_light_pref_sum(scene);

    if (!aiscene->HasCameras()) {
        LOG_WARN("scene has no cameras, using default");
        add_default_camera(scene);
    }
    scene.chosen_camera = 0;

    LOG_INFO("loaded scene: {} triangles, {} materials, {} lights, {} textures, {} cameras", scene.triangles.size(),
             scene.materials.size(), scene.lights.size(), scene.textures.size(), scene.cameras.size());
    return scene;
}