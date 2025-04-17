module;

#include <random>
#include "Serialization/SerializationMacros.h"

module Graphics:Material;

import :Material;
import :ResourceManager;
import :TextureHelpers;
import Core;
import Common;

ConstantBuffer<Material::MaterialData> Material::constantBuffer{};

Material::Material(MaterialData initialData)
    : data{ std::move(initialData) }
{}

Material::Material(const Material& other)
    : diffuseTexture(other.diffuseTexture)
    , normalTexture(other.normalTexture)
    , emissiveTexture(other.emissiveTexture)
    , aoTextureData(other.aoTextureData)
    , roughnessTextureData(other.roughnessTextureData)
    , metallicTextureData(other.metallicTextureData)
    , armTexture(other.armTexture)
    , shadingModelName(other.shadingModelName)
    , data(other.data)
{
}

Material& Material::operator=(const Material& other)
{
    if (this == &other)
        return *this;
    ILazyLoadedResource::operator =(other);
    diffuseTexture = other.diffuseTexture;
    normalTexture = other.normalTexture;
    emissiveTexture = other.emissiveTexture;
    aoTextureData = other.aoTextureData;
    roughnessTextureData = other.roughnessTextureData;
    metallicTextureData = other.metallicTextureData;
    armTexture = other.armTexture;
    shadingModelName = other.shadingModelName;
    data = other.data;
    // constantBuffer = other.constantBuffer;
    return *this;
}

void Material::LoadResource(Device& device)
{
    ILazyLoadedResource::LoadResource(device);

    auto& resourceManager = ResourceManager::Get();
    
    resourceManager.textureLoader.FillAsset(normalTexture);
    resourceManager.textureLoader.FillAsset(emissiveTexture);
    resourceManager.textureLoader.FillAsset(diffuseTexture);
    resourceManager.textureLoader.FillAsset(armTexture);

    if (!armTexture && (aoTextureData || metallicTextureData || roughnessTextureData))
    {
        armTexture = resourceManager.textureLoader.ImportFromMemory<Texture>(AssetMetadata{"ARMTexture", "", true}, TextureHelpers::BuildARMTextureFromData(device, aoTextureData, roughnessTextureData, metallicTextureData));
        aoTextureData = {};
        roughnessTextureData = {};
        metallicTextureData = {};
    }
}

#ifdef IMGUI_ENABLED

#ifdef EDITOR
import Editor;
#else
import ImGui;
#endif
import :Shader;

bool Material::ImGuiDraw()
{
    bool result = false;

    ImGui::SeparatorText("Material Parameters");

    result |= ImGui::SliderFloat("Emissive", &data.emissive, 0.0f, 1.0f);
    result |= ImGui::SliderFloat("Roughness", &data.roughness, 0.0f, 1.0f);
    result |= ImGui::SliderFloat("Metallic", &data.metallic, 0.0f, 1.0f);
    result |= ImGui::ColorEdit3("Diffuse Color", &data.diffuseColor.x);
    
    ImGui::SeparatorText("Shaders");

    result |= ScytheGui::ComboEnum("Shading Model", shadingModelName);
    ScytheGui::TextTooltip("The type of rendering to lighting to use for that material.\nLIT and UNLIT will be used in most common scenarios.");

    auto writeTextureRow = [&result](const char* label, AssetRef<Texture>& texture) {
        ImGui::PushID(label);
        if (ScytheGui::BeginLabeledField())
        {
#ifdef EDITOR
            result |= DrawAssetRefDragDrop(texture);
#else
            result |= texture.ImGuiDrawEditContent();
#endif
            ScytheGui::EndLabeledField(label);
        }
        ImGui::PopID();
    };

    ImGui::SeparatorText("Textures");
    
    writeTextureRow("Diffuse", diffuseTexture);
    writeTextureRow("Normal", normalTexture);

    writeTextureRow("Ambient Occlusion", armTexture);
    writeTextureRow("Roughness", armTexture);
    writeTextureRow("Metallic", armTexture);

    return result;
}

#endif

DEFINE_SERIALIZATION_AND_DESERIALIZATION(Material::MaterialData, diffuseColor, roughness, metallic, emissive)

BEGIN_SERIALIZATION(Material)
    WRITE_FIELD(data);
    WRITE_FIELD(shadingModelName);
    WRITE_KEY_FIELD_OPTIONAL(diffuseTexture, "diffuseTexture", diffuseTexture->GetDescription());
    WRITE_KEY_FIELD_OPTIONAL(armTexture, "armTexture", armTexture->GetDescription());
    WRITE_KEY_FIELD_OPTIONAL(emissiveTexture, "emissiveTexture", emissiveTexture->GetDescription());
    WRITE_KEY_FIELD_OPTIONAL(normalTexture, "normalTexture", normalTexture->GetDescription());
END_SERIALIZATION()

BEGIN_DESERIALIZATION(Material)
    PARSE_FIELD(data);
    PARSE_FIELD(shadingModelName);
    
    // Add texture descriptions here
END_DESERIALIZATION()

template<>
ArchiveStream& operator<<(ArchiveStream& ar, const Material::MaterialData& matDat)
{
    ar << matDat.diffuseColor;
    ar << matDat.roughness;
    ar << matDat.metallic;
    ar << matDat.emissive;
    return ar;
}

template<>
ArchiveStream& operator>>(ArchiveStream& ar, Material::MaterialData& matDat)
{
    ar >> matDat.diffuseColor;
    ar >> matDat.roughness;
    ar >> matDat.metallic;
    ar >> matDat.emissive;
    return ar;
}

template<>
ArchiveStream& operator<<(ArchiveStream& ar, const Material& mat)
{
    ar << mat.data;
    ar << mat.shadingModelName;
    ar << mat.diffuseTexture;
    ar << mat.normalTexture;
    ar << mat.emissiveTexture;
    ar << mat.armTexture;
    return ar;
}

template<>
ArchiveStream& operator>>(ArchiveStream& ar, Material& mat)
{
    ar >> mat.data;
    ar >> mat.shadingModelName;
    ar >> mat.diffuseTexture;
    ar >> mat.normalTexture;
    ar >> mat.emissiveTexture;
    ar >> mat.armTexture;
    return ar;
}

void MaterialLoaderUtils::LoadPresets(Device& device, ResourceLoader<Material>::CacheType& cache)
{
    {
        auto mat = std::make_unique<Material>();
        mat->shadingModelName = ShadingModelStore::LIT;
        mat->RegisterLazyLoadedResource(true);
        cache[Guid("Default")] = ResourceLoader<Material>::LoaderEntry{
            .metadata = AssetMetadata{
                .name = "Default",
                .isLoaded = true,
                .isTransient = true,
            },
            .asset = std::move(mat)
        };
    }

    {
        auto mat = std::make_unique<Material>();
        mat->shadingModelName = ShadingModelStore::UNLIT;
        mat->RegisterLazyLoadedResource(true);
        cache[Guid("DefaultUnlit")] = ResourceLoader<Material>::LoaderEntry{
            .metadata = AssetMetadata{
                .name = "DefaultUnlit",
                .isLoaded = true,
                .isTransient = true,
            },
            .asset = std::move(mat)
        };
    }
}

std::unique_ptr<Material> MaterialLoaderUtils::LoadAsset(ArchiveStream& loadStream)
{
    auto m = std::make_unique<Material>();
    loadStream >> *m;
    m->RegisterLazyLoadedResource(true);
    return m;
}

// This needs to be after the Material's operator<< definition
void MaterialLoaderUtils::SaveAsset(const Material& asset, ArchiveStream& saveStream, EntityComponentSystem& ecs)
{
    saveStream << asset;
}

bool MaterialLoaderUtils::HandlesExtension(const std::string& extension)
{
    return false;
}
