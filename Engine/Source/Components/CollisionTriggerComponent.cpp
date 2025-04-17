module;
#include "Physics/PhysicsSystem-Headers.hpp"

#include "Serialization/SerializationMacros.h"

module Systems.Physics:CollisionTriggerComponent;

import Systems.Physics;
import Core;
import Serialization;
import :Shapes;

CollisionTriggerComponent::CollisionTriggerComponent()
    : shapeSettings{}
{
}

CollisionTriggerComponent::CollisionTriggerComponent(const CollisionTriggerComponent& other)
    : Super(other) 
    , showDebug{other.showDebug}
    , shapeSettings{other.shapeSettings ? other.shapeSettings->Copy<Shape>() : nullptr}
{
}

CollisionTriggerComponent::~CollisionTriggerComponent()
{
}

#ifdef EDITOR
import ImGui;

bool CollisionTriggerComponent::DrawEditorInfo()
{
    bool result = false;
    result |= ImGui::Checkbox("Show Debug", &showDebug);
    ScytheGui::TextTooltip("Displays a wireframe mesh over the trigger's shape");

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

BEGIN_DESERIALIZATION(CollisionTriggerComponent)
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

BEGIN_SERIALIZATION(CollisionTriggerComponent)
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