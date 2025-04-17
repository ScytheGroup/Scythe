export module Reflection:Core;

import Common;

export import :TypeBase;

export class Reference;

export class IClass
{
public:
    virtual ~IClass() = default;

    // virtual StaticString GetName() = 0;
    //virtual void GetMethods() = 0;

    virtual Reference GetField(Reference ref, StaticString name) = 0;
    virtual std::vector<Field> GetFields() = 0;
    //virtual void GetMethod(StaticString name) = 0;
};

export struct Type;

export
namespace Reflection
{
    template <class T>
    class Class;

    template <class T>
    concept IsReflected = requires
    {
        typename Reflection::Class<T>;
        // class needs to be defined
        { Reflection::Class<T>::GetName() } -> std::convertible_to<StaticString>;
    };

    class Internal
    {
        friend Type;
        // Should never be called directly
        static std::unordered_map<uint32_t, std::unique_ptr<IClass>>& GetClasses();

    public:
        // Registers a class with the reflection system
        template <class T>
        static void Register(StaticString name)
        {
            GetClasses().emplace(name, std::make_unique<Class<T>>());
        }
    };
}
