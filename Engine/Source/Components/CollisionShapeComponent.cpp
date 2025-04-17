module;

#include "Serialization/SerializationMacros.h"

module Systems.Physics:ShapeComponent;

import :ShapeComponent;
import Systems.Physics;
import Serialization;

CollisionShapeComponent::CollisionShapeComponent()
{}

CollisionShapeComponent::CollisionShapeComponent(const CollisionShapeComponent& other)
    : Super(other) 
    , shapeSettings{other.shapeSettings ? other.shapeSettings->Copy<Shape>() : nullptr}
{
}

#if EDITOR

import ImGui;

bool CollisionShapeComponent::DrawEditorInfo()
{
    bool result = false;

    result |= ScytheGui::ComboTypes<
        Shape,
        SphereShape,
        BoxShape,
        CapsuleShape,
        MeshShape
    >("Shapes", shapeSettings);
    ScytheGui::TextTooltip("Defines the type of collision shape of this component");

    if (shapeSettings)
    {
        ImGui::SeparatorText(shapeSettings->GetTypeName());
        bool shapeChanged = shapeSettings->ImGuiDraw();
        
        if (shapeChanged)
        {
            shapeSettings->GetSettings()->ClearCachedResult();
        }
        
        result |= shapeChanged;
    }
    
    return result;
}

#endif

BEGIN_DESERIALIZATION(CollisionShapeComponent)
    auto field = (*obj.get_object().begin());
    DeserializeFieldSwitch<
        BoxShape,
        MeshShape,
        SphereShape,
        CapsuleShape
    >(field, [&]<class Wrapper>(Wrapper) {
        using ShapeType = typename Wrapper::Type;
        auto shape = std::make_unique<ShapeType>(); 
        PARSE_CHECKED(field.value(), *shape);
        PARSED_VAL.shapeSettings = std::move(shape);
        return simdjson::SUCCESS;
    });
END_DESERIALIZATION()

BEGIN_SERIALIZATION(CollisionShapeComponent)
    if (SER_VAL.shapeSettings)
    {
        SwitchClass<
            BoxShape,
            SphereShape,
            MeshShape,
            CapsuleShape
        >(*SER_VAL.shapeSettings, [&]<typename T0>(T0) {
            using ComponentType = typename T0::Type;
            json[Type::GetTypeName<ComponentType>().data()] = *((SER_VAL.shapeSettings)->Cast<ComponentType>());
        });
    }
END_SERIALIZATION()