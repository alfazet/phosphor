#ifndef PHOSPHOR_MATERIAL_PARSER_HPP
#define PHOSPHOR_MATERIAL_PARSER_HPP

#include "host/glm_bundle.hpp"
#include "host/scene_data.hpp"

#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <optional>

u32 parse_material(const aiScene *scene, aiMesh *mesh, SceneData &out_scene, std::optional<vec3> &emissive_out);

#endif // PHOSPHOR_MATERIAL_PARSER_HPP
