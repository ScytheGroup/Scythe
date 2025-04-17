module;

#include "Common/Macros.hpp"
#include "Serialization/SerializationMacros.h"

export module Components:Mesh;
import :Component;
import Graphics;
import Generated;
import Common;
import Tools;
import Serialization;

export class MeshComponent : public Component
{
    SCLASS(MeshComponent, Component);
    REQUIRES(TransformComponent);
    MANUALLY_DEFINED_EDITOR();
    friend Graphics;
public:
    MeshComponent() {}

    AssetRef<Mesh> mesh{};
    std::unordered_map<uint32_t, AssetRef<Material>> materialOverrides{};
    // Determines if the mesh should be rendered.
    bool isVisible = true;
    bool isFrustumCulled = true;
};

DECLARE_SERIALIZABLE(MeshComponent)