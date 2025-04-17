module;
#include <assimp/color4.h>
#include <assimp/material.h>
module Graphics:AssimpImportHelpers;

// For intellisense :(
import :AssimpImportHelpers;
import :TextureHelpers;

Vector4 AIColor4DToVector4(const aiColor4D& color) { return { color.r, color.g, color.b, color.a }; }

Vector3 AIVectorToVector(const aiVector3D& vec)
{
    return { vec.x, vec.y, vec.z, };
}

Vector2 AIVectorToVector(const aiVector2D& vec)
{
    return { vec.x, vec.y, };
}

Color AIColor4DToColor(const aiColor4D& color) { return { color.r, color.g, color.b, color.a }; }

void ImportMaterialData(Material::MaterialData& materialData, aiMaterial& aiMaterial)
{
    if (aiColor4D kd; aiReturn_SUCCESS == aiGetMaterialColor(&aiMaterial, AI_MATKEY_BASE_COLOR, &kd))
    {
        materialData.diffuseColor = AIColor4DToColor(kd);
    }
    
    if (float roughness; aiReturn_SUCCESS == aiGetMaterialFloat(&aiMaterial, AI_MATKEY_ROUGHNESS_FACTOR, &roughness))
    {
        materialData.roughness = roughness;
    }

    if (float metallic; aiReturn_SUCCESS == aiGetMaterialFloat(&aiMaterial, AI_MATKEY_METALLIC_FACTOR, &metallic))
    {
        materialData.metallic = metallic;
    }

    if (float emissive; aiReturn_SUCCESS == aiGetMaterialFloat(&aiMaterial, AI_MATKEY_EMISSIVE_INTENSITY, &emissive))
    {
        materialData.emissive = emissive;
    }
}

void ImportMaterialTexturesData(Material& material, aiMaterial& aiMaterial, const std::filesystem::path& meshFolder)
{
    auto getTexturePath = [&](const std::vector<aiTextureType>& textureTypes) -> std::optional<std::filesystem::path>
    {
        for (auto textureType : textureTypes)
        {
            // For now we load all textures but only use the last one, eventually we could expand model to use many textures (lol probably not)
            for (uint32_t textureId = 0; textureId < aiMaterial.GetTextureCount(textureType); ++textureId)
            {
                aiString path;
                if (aiReturn_SUCCESS == aiMaterial.GetTexture(textureType, textureId, &path))
                {
                    return meshFolder / path.C_Str();
                }
            }            
        }

        return {};
    };
    
    std::optional<std::filesystem::path> aoPath = getTexturePath({ aiTextureType_LIGHTMAP, aiTextureType_AMBIENT, aiTextureType_AMBIENT_OCCLUSION });
    std::optional<std::filesystem::path> metallicPath = getTexturePath({ aiTextureType_METALNESS });
    std::optional<std::filesystem::path> roughnessPath = getTexturePath({ aiTextureType_DIFFUSE_ROUGHNESS });
    std::optional<std::filesystem::path> normalPath = getTexturePath({ aiTextureType_NORMALS, aiTextureType_NORMAL_CAMERA });
    std::optional<std::filesystem::path> baseColorPath = getTexturePath({ aiTextureType_BASE_COLOR });
    std::optional<std::filesystem::path> emissionColorPath = getTexturePath({ aiTextureType_EMISSION_COLOR, aiTextureType_EMISSIVE });

    auto createTextureData = [](const std::filesystem::path& path)
    {
        return Texture::TextureData(path, Texture::TEXTURE_2D, Texture::DEFAULT, Texture::TEXTURE_SRV | Texture::TEXTURE_RTV, true);        
    };
    
    auto registerTexture = [&](AssetRef<Texture>& ref, const std::filesystem::path& path)
    {
        ref = ResourceLoader<Texture>::Get().ImportFromMemory(
            AssetMetadata::CreateFromPath(path),
            createTextureData(path)
        );
    };
    
    if (aoPath && metallicPath && roughnessPath && aoPath == metallicPath && metallicPath == roughnessPath)
    {
        registerTexture(material.armTexture, *aoPath);
    }
    else
    {
        if (aoPath) material.aoTextureData = createTextureData(*aoPath);
        if (metallicPath) material.metallicTextureData = createTextureData(*metallicPath);
        if (roughnessPath) material.roughnessTextureData = createTextureData(*roughnessPath);
    }

    if (normalPath) registerTexture(material.normalTexture, *normalPath);
    if (emissionColorPath) registerTexture(material.emissiveTexture, *emissionColorPath);
    
    if (baseColorPath)
    {
        Texture::TextureData data = createTextureData(*baseColorPath);
        data.description.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        material.diffuseTexture = ResourceLoader<Texture>::Get().ImportFromMemory(
            AssetMetadata::CreateFromPath(*baseColorPath),
            std::move(data)
        );
    }
}

