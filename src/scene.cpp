#include "scene.hpp"
#include "constants.h"
#include "glm_bundle.hpp"
#include "logger.hpp"
#include "stb_image.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <filesystem>

mat4 mat4_assimp_to_glm(const aiMatrix4x4 &from) {
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

Material make_default_material() {
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
    mat.trans_tex_index = NO_TEXTURE;

    mat.uv_offset = float2{{0.0f, 0.0f}};
    mat.uv_scale = float2{{1.0f, 1.0f}};
    mat.uv_rotation = 0.0f;

    mat.att_color = float4{{1.0f, 1.0f, 1.0f, 0.0f}};
    mat.att_dist = INF;
    mat.thickness = 0.0f;

    return mat;
}

void parse_pbr_metallic_roughness(aiMaterial *ai_mat, const SceneData &scene, Material &out) {
    aiUVTransform ai_uv{};
    if (ai_mat->Get(AI_MATKEY_UVTRANSFORM(aiTextureType_DIFFUSE, 0), ai_uv) == AI_SUCCESS) {
        out.uv_offset = float2{{ai_uv.mTranslation.x, ai_uv.mTranslation.y}};
        out.uv_scale = float2{{ai_uv.mScaling.x, ai_uv.mScaling.y}};
        out.uv_rotation = ai_uv.mRotation;
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
            out.trans_tex_index = *idx;
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
    emissive_out = std::nullopt;
    if (mesh->mMaterialIndex >= scene->mNumMaterials) {
        u32 idx = out_scene.materials.size();
        out_scene.materials.push_back(mat);
        return idx;
    }

    aiMaterial *ai_mat = scene->mMaterials[mesh->mMaterialIndex];

    parse_pbr_metallic_roughness(ai_mat, out_scene, mat);
    parse_khr_transmission(ai_mat, out_scene, mat);
    parse_khr_ior(ai_mat, mat);
    parse_khr_volume(ai_mat, mat);
    parse_material_textures(ai_mat, out_scene, mat);

    vec3 emissive_vec(mat.emissive.x, mat.emissive.y, mat.emissive.z);
    if (glm::any(glm::greaterThan(emissive_vec, vec3(EPS))))
        emissive_out = emissive_vec;
    u32 idx = out_scene.materials.size();
    out_scene.materials.push_back(mat);

    return idx;
}

void parse_mesh(aiMesh *ai_mesh, const aiScene *ai_scene, SceneData &out_scene, mat4 global_transform) {
    mat3 normal_matrix = glm::transpose(glm::inverse(mat3(global_transform)));

    std::optional<vec3> emissive;
    u32 mat_index = parse_material(ai_scene, ai_mesh, out_scene, emissive);

    u32 triangle_start = static_cast<u32>(out_scene.triangles.size());
    for (u32 f = 0; f < ai_mesh->mNumFaces; f++) {
        aiFace face = ai_mesh->mFaces[f];
        aiVector3D v0 = ai_mesh->mVertices[face.mIndices[0]];
        aiVector3D v1 = ai_mesh->mVertices[face.mIndices[1]];
        aiVector3D v2 = ai_mesh->mVertices[face.mIndices[2]];
        vec3 p0 = vec3(global_transform * vec4(v0.x, v0.y, v0.z, 1.0f));
        vec3 p1 = vec3(global_transform * vec4(v1.x, v1.y, v1.z, 1.0f));
        vec3 p2 = vec3(global_transform * vec4(v2.x, v2.y, v2.z, 1.0f));

        vec2 uv0(0.0f), uv1(0.0f), uv2(0.0f);
        if (ai_mesh->mTextureCoords[0]) {
            aiVector3D t0 = ai_mesh->mTextureCoords[0][face.mIndices[0]];
            aiVector3D t1 = ai_mesh->mTextureCoords[0][face.mIndices[1]];
            aiVector3D t2 = ai_mesh->mTextureCoords[0][face.mIndices[2]];
            uv0 = vec2(t0.x, t0.y);
            uv1 = vec2(t1.x, t1.y);
            uv2 = vec2(t2.x, t2.y);
        }

        vec3 n0, n1, n2;
        if (ai_mesh->HasNormals()) {
            aiVector3D an0 = ai_mesh->mNormals[face.mIndices[0]];
            aiVector3D an1 = ai_mesh->mNormals[face.mIndices[1]];
            aiVector3D an2 = ai_mesh->mNormals[face.mIndices[2]];
            n0 = glm::normalize(normal_matrix * vec3(an0.x, an0.y, an0.z));
            n1 = glm::normalize(normal_matrix * vec3(an1.x, an1.y, an1.z));
            n2 = glm::normalize(normal_matrix * vec3(an2.x, an2.y, an2.z));
        } else {
            vec3 face_normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
            n0 = n1 = n2 = face_normal;
        }

        vec3 t0, t1, t2;
        if (ai_mesh->mTangents) {
            aiVector3D at0 = ai_mesh->mTangents[face.mIndices[0]];
            aiVector3D at1 = ai_mesh->mTangents[face.mIndices[1]];
            aiVector3D at2 = ai_mesh->mTangents[face.mIndices[2]];
            t0 = glm::normalize(normal_matrix * vec3(at0.x, at0.y, at0.z));
            t1 = glm::normalize(normal_matrix * vec3(at1.x, at1.y, at1.z));
            t2 = glm::normalize(normal_matrix * vec3(at2.x, at2.y, at2.z));
        } else {
            vec3 edge1 = p1 - p0, edge2 = p2 - p0;
            vec2 duv1 = uv1 - uv0, duv2 = uv2 - uv0;
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

        Triangle tri{};
        tri.v0 = vec3_to_float4(p0);
        tri.v1 = vec3_to_float4(p1);
        tri.v2 = vec3_to_float4(p2);
        tri.n0 = vec3_to_float4(n0);
        tri.n1 = vec3_to_float4(n1);
        tri.n2 = vec3_to_float4(n2);
        tri.t0 = vec3_to_float4(t0);
        tri.t1 = vec3_to_float4(t1);
        tri.t2 = vec3_to_float4(t2);
        tri.uv0 = vec2_to_float2(uv0);
        tri.uv1 = vec2_to_float2(uv1);
        tri.uv2 = vec2_to_float2(uv2);
        tri.mat_index = mat_index;
        out_scene.triangles.push_back(tri);
    }

    u32 triangle_count = static_cast<u32>(out_scene.triangles.size()) - triangle_start;
    u32 emissive_triangles_start = out_scene.emissive_triangles.size();
    if (emissive.has_value() && triangle_count > 0) {
        const Material &m = out_scene.materials[mat_index];
        f32 total_area = 0.0f;
        for (u32 j = triangle_start; j < triangle_start + triangle_count; j++) {
            const Triangle &tri = out_scene.triangles[j];
            out_scene.emissive_triangles.push_back(tri);
            vec3 e1(tri.v1.x - tri.v0.x, tri.v1.y - tri.v0.y, tri.v1.z - tri.v0.z);
            vec3 e2(tri.v2.x - tri.v0.x, tri.v2.y - tri.v0.y, tri.v2.z - tri.v0.z);
            total_area += 0.5f * glm::length(glm::cross(e1, e2));
        }
        vec3 emissive_power = PI * (*emissive) * total_area;
        out_scene.lights.push_back(
            make_textured_light(m.emis_index, emissive_triangles_start, triangle_count, emissive_power));
    }
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
    f32 fov_h_deg = ai_camera->mHorizontalFOV * (180.0f / static_cast<f32>(PI));
    f32 aspect = ai_camera->mAspect > 0.0f ? ai_camera->mAspect : DEFAULT_CAMERA_ASPECT;

    out_scene.cameras.emplace_back(position, look_at_world, up, fov_h_deg, aspect);
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

void parse_node(aiNode *ai_node, const aiScene *ai_scene, SceneData &out_scene, mat4 parent_transform) {
    mat4 local_transform = mat4_assimp_to_glm(ai_node->mTransformation);
    mat4 global_transform = parent_transform * local_transform;

    for (u32 i = 0; i < ai_scene->mNumCameras; i++) {
        aiCamera *ai_cam = ai_scene->mCameras[i];
        if (ai_cam->mName == ai_node->mName)
            parse_camera(ai_cam, out_scene, global_transform);
    }

    for (u32 i = 0; i < ai_scene->mNumLights; i++) {
        aiLight *ai_light = ai_scene->mLights[i];
        if (ai_light->mName == ai_node->mName)
            parse_light(ai_light, out_scene, global_transform);
    }

    for (u32 i = 0; i < ai_node->mNumMeshes; i++) {
        aiMesh *ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
        parse_mesh(ai_mesh, ai_scene, out_scene, global_transform);
    }

    for (u32 i = 0; i < ai_node->mNumChildren; i++)
        parse_node(ai_node->mChildren[i], ai_scene, out_scene, global_transform);
}

SceneData read_gltf_scene(const char *file_name) {
    TimerScope timer_scope("loading scene");

    SceneData scene{};
    Assimp::Importer importer;
    const aiScene *ai_scene = importer.ReadFile(file_name, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                                                               aiProcess_CalcTangentSpace);

    if (!ai_scene || (ai_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !ai_scene->mRootNode) {
        LOG_ERROR("while parsing scene {}: {}", file_name, importer.GetErrorString());
        return scene;
    }

    std::filesystem::path p(file_name);
    std::string texture_dir = p.parent_path().string();
    parse_textures(ai_scene, scene, texture_dir.c_str());

    mat4 identity(1.0f);
    parse_node(ai_scene->mRootNode, ai_scene, scene, identity);
    scene.build_luminance_pref_sum();

    if (!ai_scene->HasCameras()) {
        LOG_FATAL("scene has no cameras");
        // TODO: bring back the default camera on the edge of the bbox and change this to a warning
    }
    scene.chosen_camera = 0;

    LOG_INFO("loaded scene: {} triangles, {} materials, {} lights, {} textures, {} cameras", scene.triangles.size(),
             scene.materials.size(), scene.lights.size(), scene.textures.size(), scene.cameras.size());
    return scene;
}

const Camera &SceneData::get_camera() const { return this->cameras[*this->chosen_camera]; }

void SceneData::build_luminance_pref_sum() {
    this->luminance_pref_sum.clear();
    this->luminance_pref_sum.reserve(this->lights.size());
    f32 running = 0.0f;
    for (const auto &l : this->lights) {
        f32 luminance = l.power.x * 0.2126f + l.power.y * 0.7152f + l.power.z * 0.0722f;
        running += std::max(luminance, 0.0f);
        this->luminance_pref_sum.push_back(running);
    }
}
