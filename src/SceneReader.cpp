#include "SceneReader.hpp"
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void processNode(aiNode* node, const aiScene* aiscene, Scene& out_scene, mat4 currentTransform);

std::vector<Scene> ReadFile(const char* fileName) {
    std::vector<Scene> scenes;

    Assimp::Importer importer;
    const aiScene* aiscene = importer.ReadFile(fileName,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices
    );

    auto parsedScene = Scene();

    if (!aiscene || aiscene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiscene->mRootNode) {
        std::cerr << "Assimp Error: " << importer.GetErrorString() << std::endl;
        return scenes;
    }

    // Camera
    Camera engineCamera;
    // TODO add support for many cameras in scene
    if (aiscene->HasCameras()) {
        aiCamera* ai_cam = aiscene->mCameras[0];

        vec3 position(ai_cam->mPosition.x, ai_cam->mPosition.y, ai_cam->mPosition.z);
        vec3 lookAt(ai_cam->mLookAt.x, ai_cam->mLookAt.y, ai_cam->mLookAt.z);
        vec3 up(ai_cam->mUp.x, ai_cam->mUp.y, ai_cam->mUp.z);

        f32 fov_h = ai_cam->mHorizontalFOV;
        f32 aspect = ai_cam->mAspect;
        if (aspect <= 0.0f) {
            aspect = 16.0f / 9.0f;
        }
        f32 fov_v_rad = 2.0f * std::atan(std::tan(fov_h / 2.0f) / aspect);
        f32 fov_v_deg = fov_v_rad * (180.0f / M_PI);

        engineCamera = Camera(position, lookAt, up, fov_v_deg);
    } else {
        engineCamera = Camera(vec3(0, 0, 5), vec3(0, 0, 0), vec3(0, 1, 0), 45.0f);
    }

    parsedScene.set_camera(engineCamera);

    // lights
    if (aiscene->HasLights()) {
        for (unsigned int i = 0; i < aiscene->mNumLights; ++i) {
            aiLight* ai_light = aiscene->mLights[i];

            if (ai_light->mType == aiLightSource_POINT) {
                vec3 position(ai_light->mPosition.x, ai_light->mPosition.y, ai_light->mPosition.z);

                // TODO PointLight change is needed; hardcoded for now
                PointLight engineLight(position, 1000.0f);
                parsedScene.add_light(engineLight);
            }
        }
    }

    // meshes
    mat4 identity(1.0f);
    processNode(aiscene->mRootNode, aiscene, parsedScene, identity);

    scenes.push_back(parsedScene);
    return scenes;
}

mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& from) {
    mat4 to;
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

// parsing the tree
void processNode(aiNode* node, const aiScene* aiscene, Scene& out_scene, mat4 parentTransform) {
    mat4 localTransform = aiMatrix4x4ToGlm(node->mTransformation);
    mat4 globalTransform = parentTransform * localTransform;
    Material white {vec3(0.8f, 0.8f, 0.8f), vec3(0.0f, 0.0f, 0.0f)};

    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = aiscene->mMeshes[node->mMeshes[i]];

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            aiFace face = mesh->mFaces[f];

            aiVector3D v0 = mesh->mVertices[face.mIndices[0]];
            aiVector3D v1 = mesh->mVertices[face.mIndices[1]];
            aiVector3D v2 = mesh->mVertices[face.mIndices[2]];

            vec3 p0 = vec3(globalTransform * vec4(v0.x, v0.y, v0.z, 1.0f));
            vec3 p1 = vec3(globalTransform * vec4(v1.x, v1.y, v1.z, 1.0f));
            vec3 p2 = vec3(globalTransform * vec4(v2.x, v2.y, v2.z, 1.0f));

            // TODO material hardcoded here as well
            // before taking material from gltf format changing has to be figured out
            // normals, textures too
            Triangle triangle(p0, p1, p2, white);
            out_scene.add_triangle(triangle);
        }
    }

    // process any children nodes
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processNode(node->mChildren[i], aiscene, out_scene, globalTransform);
    }
}