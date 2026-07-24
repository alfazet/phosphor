#include "scene_reader.hpp"
#include "logger.hpp"
#include "stb_image.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <filesystem>
#include <iostream>

void process_node(aiNode *node, const aiScene *aiscene, Scene &out_scene, mat4 current_transform);

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

static void parse_camera(const aiScene *aiscene, const aiCamera *ai_camera, Scene &out_scene, mat4 global_transform) {
    vec3 position =
        vec3(global_transform * vec4(ai_camera->mPosition.x, ai_camera->mPosition.y, ai_camera->mPosition.z, 1.0f));
    vec3 lookAtDir =
        vec3(global_transform * vec4(ai_camera->mLookAt.x, ai_camera->mLookAt.y, ai_camera->mLookAt.z, 0.0f));
    // http://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices/ why 0.0f 1.0f
    vec3 lookAtWorld = position + lookAtDir;

    vec3 up = glm::normalize(vec3(global_transform * vec4(ai_camera->mUp.x, ai_camera->mUp.y, ai_camera->mUp.z, 0.0f)));

    f32 fov_h = ai_camera->mHorizontalFOV;
    f32 aspect = ai_camera->mAspect;

    Camera engineCamera(position, lookAtWorld, up, (fov_h * (180.0f / M_PI)), aspect);
    out_scene.add_camera(engineCamera);
}

static void parse_light(const aiScene *aiscene, const aiLight *ai_light, Scene &out_scene, mat4 global_transform) {
    if (ai_light->mType == aiLightSource_POINT) {
        vec3 position =
            vec3(global_transform * vec4(ai_light->mPosition.x, ai_light->mPosition.y, ai_light->mPosition.z, 1.0f));
        // TODO: PointLight change is needed; hardcoded for now
        PointLight engineLight(position, vec3(100.0f, 100.0f, 100.0f));
        out_scene.add_point_light(engineLight);
        return;
    }
    LOG_WARN("scene has unsupported light type: {}", static_cast<i32>(ai_light->mType));
}

usize find_texture(std::string name, const std::vector<Texture> &textures) {
    for (u32 i = 0; i < textures.size(); i++) {
        if (textures[i].name == name) {
            return i;
        }
    }
    LOG_FATAL("texture {} not found", name);
}

Material parse_material(const aiScene *scene, aiMesh *mesh, const std::vector<Texture> &textures,
                        std::optional<usize> &emis_index) {
    Material mat = Material{vec4(1.0f, 1.0f, 1.0f, 1.0f), 0.0f, 1.0f, BLACK};

    if (mesh->mMaterialIndex < 0)
        return mat;

    aiMaterial *ai_mat = scene->mMaterials[mesh->mMaterialIndex];

    aiColor4D base_color(1.0f, 1.0f, 1.0f, 1.0f);
    if (ai_mat->Get(AI_MATKEY_BASE_COLOR, base_color) == AI_SUCCESS)
        mat.base_color = vec4(base_color.r, base_color.g, base_color.b, base_color.a);

    ai_mat->Get(AI_MATKEY_METALLIC_FACTOR, mat.metallic);
    ai_mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, mat.roughness);

    aiColor3D emissive(0.0f, 0.0f, 0.0f);
    if (ai_mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS)
        mat.emissive = vec3(emissive.r, emissive.g, emissive.b);

    aiString path;
    if (ai_mat->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
        std::string name = std::filesystem::path(path.C_Str()).filename().string();
        mat.diff_index = find_texture(name, textures);
    }
    if (ai_mat->GetTexture(aiTextureType_EMISSIVE, 0, &path) == AI_SUCCESS) {
        std::string name = std::filesystem::path(path.C_Str()).filename().string();
        mat.emis_index = find_texture(name, textures);
        emis_index = mat.emis_index;
    }
    if (ai_mat->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS) {
        std::string name = std::filesystem::path(path.C_Str()).filename().string();
        mat.norm_index = find_texture(name, textures);
    }
    if (ai_mat->GetTexture(aiTextureType_METALNESS, 0, &path) == AI_SUCCESS ||
        ai_mat->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &path) == AI_SUCCESS) {
        std::string name = std::filesystem::path(path.C_Str()).filename().string();
        mat.metal_rough_index = find_texture(name, textures);
    }
    if (ai_mat->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &path) == AI_SUCCESS) {
        std::string name = std::filesystem::path(path.C_Str()).filename().string();
        mat.occlusion_index = find_texture(name, textures);
    }
    return mat;
}

void load_texture(const aiScene *aiscene, aiMaterial *mat, aiTextureType type, const char *directory,
                  Scene &out_scene) {
    aiString path;
    aiReturn result = mat->GetTexture(type, 0, &path);
    if (result != AI_SUCCESS) {
        return;
    }

    LOG_INFO("loading texture from {} of type {}", path.C_Str(), aiTextureTypeToString(type));
    Texture t;
    t.name = std::filesystem::path(path.C_Str()).filename().string();
    t.channels = 3;
    aiUVTransform ai_transform;
    if (mat->Get(AI_MATKEY_UVTRANSFORM(type, 0), ai_transform) == AI_SUCCESS) {
        UVTransform ut;
        ut.offset = vec2(ai_transform.mTranslation.x, ai_transform.mTranslation.y);
        ut.rotation = ai_transform.mRotation;
        ut.scale = vec2(ai_transform.mScaling.x, ai_transform.mScaling.y);
        t.uv_transform = ut;
    }

    const aiTexture *embedded_tex = aiscene->GetEmbeddedTexture(path.C_Str());

    // load embedded
    if (embedded_tex) {
        i32 w, h, c;
        u8 *raw = nullptr;

        if (embedded_tex->mHeight == 0) {
            // compressed
            const u8 *buf = reinterpret_cast<const u8 *>(embedded_tex->pcData);
            raw = stbi_load_from_memory(buf, embedded_tex->mWidth, &w, &h, &c, 3);
            if (!raw) {
                LOG_FATAL("failed to decode embedded texture {}", path.C_Str());
            }
        } else {
            // uncompressed
            w = embedded_tex->mWidth;
            h = embedded_tex->mHeight;

            std::vector<u8> converted(w * h * 3);
            const aiTexel *texels = embedded_tex->pcData;
            for (int i = 0; i < w * h; ++i) {
                converted[i * 3 + 0] = texels[i].r;
                converted[i * 3 + 1] = texels[i].g;
                converted[i * 3 + 2] = texels[i].b;
            }
            t.width = w;
            t.height = h;
            t.data = std::move(converted);
            out_scene.add_texture(t);
            return;
        }

        t.width = w;
        t.height = h;
        t.data.assign(raw, raw + (w * h * 3));
        stbi_image_free(raw);
        out_scene.add_texture(t);
        return;
    }

    // load from file
    std::string fullPath = std::string(directory) + "/" + path.C_Str();
    i32 w, h, c;
    u8 *raw = stbi_load(fullPath.c_str(), &w, &h, &c, 3);
    if (!raw) {
        LOG_FATAL("failed to load texture from {}", path.C_Str());
    }

    t.width = w;
    t.height = h;
    t.data.assign(raw, raw + (w * h * 3));
    stbi_image_free(raw);

    out_scene.add_texture(t);
}

void parse_textures(const aiScene *aiscene, Scene &out_scene, const char *directory) {
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

std::vector<Scene> read_file(const char *file_name, RngState rng) {
    TimerScope timer_scope("loading scenes");

    std::vector<Scene> scenes;
    Assimp::Importer importer;
    const aiScene *aiscene = importer.ReadFile(file_name, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                                                              aiProcess_CalcTangentSpace);

    auto parsed_scene = Scene();

    if (!aiscene || aiscene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiscene->mRootNode) {
        LOG_ERROR("while parsing scenes: {}", importer.GetErrorString());
        return scenes;
    }

    std::filesystem::path p(file_name);
    std::string texture_path = p.parent_path().string();
    parse_textures(aiscene, parsed_scene, texture_path.c_str());

    mat4 identity(1.0f);
    process_node(aiscene->mRootNode, aiscene, parsed_scene, identity);
    if (!aiscene->HasCameras()) {
        LOG_WARN("scene has no cameras, using default");
        parsed_scene.add_default_camera();
    }
    parsed_scene.set_camera(0);
    scenes.push_back(parsed_scene);

    LOG_INFO("loaded {} scene(s)", scenes.size());

    return scenes;
}

// parsing the tree
void process_node(aiNode *node, const aiScene *aiscene, Scene &out_scene, mat4 parent_transform) {
    mat4 local_transform = ai_matrix4x4_to_glm(node->mTransformation);
    mat4 global_transform = parent_transform * local_transform;

    // check if node is a camera
    for (u32 i = 0; i < aiscene->mNumCameras; i++) {
        aiCamera *ai_cam = aiscene->mCameras[i];
        if (ai_cam->mName == node->mName)
            parse_camera(aiscene, ai_cam, out_scene, global_transform);
    }

    // check if node is a light
    for (u32 i = 0; i < aiscene->mNumLights; i++) {
        aiLight *ai_light = aiscene->mLights[i];
        if (ai_light->mName == node->mName)
            parse_light(aiscene, ai_light, out_scene, global_transform);
    }

    // handle meshes
    for (usize i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = aiscene->mMeshes[node->mMeshes[i]];

        mat3 normal_matrix = glm::transpose(glm::inverse(mat3(global_transform)));

        std::optional<usize> emis_tex_index;
        Material mat = parse_material(aiscene, mesh, out_scene.textures_, emis_tex_index);
        u32 triangle_start = static_cast<u32>(out_scene.objects.triangles_.size());

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
            vec3 n0(0.0f), n1(0.0f), n2(0.0f);
            if (mesh->HasNormals()) {
                aiVector3D an0 = mesh->mNormals[face.mIndices[0]];
                aiVector3D an1 = mesh->mNormals[face.mIndices[1]];
                aiVector3D an2 = mesh->mNormals[face.mIndices[2]];

                n0 = glm::normalize(normal_matrix * vec3(an0.x, an0.y, an0.z));
                n1 = glm::normalize(normal_matrix * vec3(an1.x, an1.y, an1.z));
                n2 = glm::normalize(normal_matrix * vec3(an2.x, an2.y, an2.z));
            } else {
                vec3 faceNormal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
                n0 = n1 = n2 = faceNormal;
            }

            vec3 t0(0.0f), t1(0.0f), t2(0.0f);
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
                vec3 faceTangent;
                if (glm::abs(det) < EPS) {
                    faceTangent = vec3(1, 0, 0); //
                } else {
                    f32 f = 1.0f / det;
                    faceTangent = f * (duv2.y * edge1 - duv1.y * edge2);
                }
                faceTangent = glm::normalize(normal_matrix * faceTangent);
                t0 = t1 = t2 = faceTangent;
            }

            Triangle triangle(p0, p1, p2, mat, uv0, uv1, uv2, n0, n1, n2, t0, t1, t2);
            out_scene.add_triangle(triangle);
        }

        if (emis_tex_index.has_value()) {
            u32 triangle_count = static_cast<u32>(out_scene.objects.triangles_.size()) - triangle_start;
            if (triangle_count > 0) {
                f32 total_area = 0.0f;
                for (u32 k = 0; k < triangle_count; k++)
                    total_area += out_scene.objects.triangles_[triangle_start + k].area();
                TexturedLight light;
                light.tex_index = *emis_tex_index;
                light.triangle_start = triangle_start;
                light.triangle_count = triangle_count;
                light.total_area = total_area;
                out_scene.add_textured_light(light);
            }
        }
    }

    // parse children
    for (usize i = 0; i < node->mNumChildren; i++) {
        process_node(node->mChildren[i], aiscene, out_scene, global_transform);
    }
}
