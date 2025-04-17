module;
#include <assimp/color4.h>
#include <assimp/material.h>
export module Graphics:AssimpImportHelpers;

import Common;

import :Material;

export Color AIColor4DToColor(const aiColor4D& color);
export Vector4 AIColor4DToVector4(const aiColor4D& color);

export Vector3 AIVectorToVector(const aiVector3D& color);
export Vector2 AIVectorToVector(const aiVector2D& color);

export void ImportMaterialData(Material::MaterialData& materialData, aiMaterial& aiMaterial);

export void ImportMaterialTexturesData(Material& material, aiMaterial& aiMaterial, const std::filesystem::path& meshPath);
