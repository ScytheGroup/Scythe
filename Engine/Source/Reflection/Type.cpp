module Reflection:Type;
import :Reference;

using namespace Reflection;

std::vector<Field> Type::GetFields(Type type)
{
    static std::unordered_map<uint32_t, std::unique_ptr<IClass>>& classes = Internal::GetClasses();
    if (classes.contains(type))
    {
        return classes[type]->GetFields();
    }
    return std::vector<Field>{};
}

Reference Type::GetField(Reference object, std::string_view name) const 
{
    return Internal::GetClasses().at(concreteTypeId)->GetField(object, name);
}
