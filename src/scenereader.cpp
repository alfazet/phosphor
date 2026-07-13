#include "scenereader.hpp"
#include "stb_image.h"

#include <filesystem>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

void processNode(aiNode *node, const aiScene *aiscene, Scene &out_scene, mat4 currentTransform);
void processTextures(const aiScene *aiscene, Scene &out_scene, const char *directory);

mat4 aiMatrix4x4ToGlm(const aiMatrix4x4 &from) {
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

void parse_camera(const aiScene *aiscene, const aiCamera *ai_camera, Scene &out_scene, mat4 global_transform) {
    aiNode *camNode = aiscene->mRootNode->FindNode(ai_camera->mName);

    vec3 position =
        vec3(global_transform * vec4(ai_camera->mPosition.x, ai_camera->mPosition.y, ai_camera->mPosition.z, 1.0f));
    vec3 lookAtDir =
        vec3(global_transform * vec4(ai_camera->mLookAt.x, ai_camera->mLookAt.y, ai_camera->mLookAt.z, 0.0f));
    // http://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices/ why 0.0f 1.0f
    vec3 lookAtWorld = position + lookAtDir;

    vec3 up = glm::normalize(vec3(global_transform * vec4(ai_camera->mUp.x, ai_camera->mUp.y, ai_camera->mUp.z, 0.0f)));

    f32 fov_h = ai_camera->mHorizontalFOV;
    f32 aspect = ai_camera->mAspect;

    Camera engineCamera;
    engineCamera = Camera(position, lookAtWorld, up, (fov_h * (180.0f / M_PI)), aspect);
    out_scene.add_camera(engineCamera);
}

void parse_light(const aiScene *aiscene, const aiLight *ai_light, Scene &out_scene, mat4 global_transform) {
    if (ai_light->mType == aiLightSource_POINT) {
        vec3 position =
            vec3(global_transform * vec4(ai_light->mPosition.x, ai_light->mPosition.y, ai_light->mPosition.z, 1.0f));
        // TODO PointLight change is needed; hardcoded for now
        PointLight engineLight(position, vec3(1000.0f, 1000.0f, 1000.0f));
        out_scene.add_point_light(engineLight);
    }
}

std::vector<Scene> ReadFile(const char *fileName) {
    std::vector<Scene> scenes;

    Assimp::Importer importer;
    const aiScene *aiscene = importer.ReadFile(fileName, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

    auto parsedScene = Scene();

    if (!aiscene || aiscene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiscene->mRootNode) {
        std::cerr << "Assimp Error: " << importer.GetErrorString() << std::endl;
        return scenes;
    }

    std::filesystem::path p(fileName);
    std::string texture_path = p.parent_path().string();
    processTextures(aiscene, parsedScene, texture_path.c_str());

    mat4 identity(1.0f);
    processNode(aiscene->mRootNode, aiscene, parsedScene, identity);

    if (aiscene->HasCameras()) {
        parsedScene.set_camera(0);
    }
    scenes.push_back(parsedScene);
    return scenes;
}

void processTextures(const aiScene *aiscene, Scene &out_scene, const char *directory) {
    for (u32 i = 0; i < aiscene->mNumMaterials; ++i) {
        aiMaterial *mat = aiscene->mMaterials[i];
        aiString path;

        aiReturn result = mat->GetTexture(aiTextureType_DIFFUSE, 0, &path);
        if (result == AI_SUCCESS) {
            printf("loading texture from %s\n", path.C_Str());
            std::string fullPath = std::string(directory) + "/" + path.C_Str();
            try {
                out_scene.add_texture(load(fullPath));
            } catch (const std::runtime_error &e) {
                printf("[ERROR]: couldn't load texture: %s\n", e.what());
            }
        }
    }
}

// parsing the tree
void processNode(aiNode *node, const aiScene *aiscene, Scene &out_scene, mat4 parentTransform) {
    mat4 localTransform = aiMatrix4x4ToGlm(node->mTransformation);
    mat4 globalTransform = parentTransform * localTransform;

    // check if node is a camera
    if (aiscene->HasCameras()) {
        for (u32 i = 0; i < aiscene->mNumCameras; ++i) {
            aiCamera *ai_cam = aiscene->mCameras[i];
            if (ai_cam->mName == node->mName)
                parse_camera(aiscene, ai_cam, out_scene, globalTransform);
        }
    }

    // check if node is a light
    if (aiscene->HasLights()) {
        for (u32 i = 0; i < aiscene->mNumLights; ++i) {
            aiLight *ai_light = aiscene->mLights[i];
            if (ai_light->mName == node->mName)
                parse_light(aiscene, ai_light, out_scene, globalTransform);
        }
    }

    Material white{vec3(0.8f, 0.8f, 0.8f), vec3(0.0f, 0.0f, 0.0f)};

    // handle meshes
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh *mesh = aiscene->mMeshes[node->mMeshes[i]];

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            aiFace face = mesh->mFaces[f];

            aiVector3D v0 = mesh->mVertices[face.mIndices[0]];
            aiVector3D v1 = mesh->mVertices[face.mIndices[1]];
            aiVector3D v2 = mesh->mVertices[face.mIndices[2]];

            vec3 p0 = vec3(globalTransform * vec4(v0.x, v0.y, v0.z, 1.0f));
            vec3 p1 = vec3(globalTransform * vec4(v1.x, v1.y, v1.z, 1.0f));
            vec3 p2 = vec3(globalTransform * vec4(v2.x, v2.y, v2.z, 1.0f));

            vec2 uv0(0.0f), uv1(0.0f), uv2(0.0f);
            if (mesh->mTextureCoords[0]) {
                aiVector3D t0 = mesh->mTextureCoords[0][face.mIndices[0]];
                aiVector3D t1 = mesh->mTextureCoords[0][face.mIndices[1]];
                aiVector3D t2 = mesh->mTextureCoords[0][face.mIndices[2]];
                uv0 = vec2(t0.x, t0.y);
                uv1 = vec2(t1.x, t1.y);
                uv2 = vec2(t2.x, t2.y);
            }

            // TODO no matertials yet
            Triangle triangle(p0, p1, p2, white, uv0, uv1, uv2, 0);
            out_scene.add_triangle(triangle);
        }
    }

    // parse children
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processNode(node->mChildren[i], aiscene, out_scene, globalTransform);
    }
}

Material parse_material(const aiScene *scene, aiMesh *mesh) {
    Material mat{
        vec3(0.8f, 0.8f, 0.8f), // diffuse
        vec3(0.0f),             // specular
        vec3(0.0f)              // emissive
    };

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial *ai_mat = scene->mMaterials[mesh->mMaterialIndex];

        aiColor3D emissive(0.0f, 0.0f, 0.0f);
        if (ai_mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS) {
            mat.emissive = vec3(emissive.r, emissive.g, emissive.b);
        }
    }

    return mat;
}
