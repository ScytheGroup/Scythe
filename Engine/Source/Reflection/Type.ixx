module;
#include <ctti/nameof.hpp>
export module Reflection:Type;

import Common;
export import :Core;

export struct Type
{
    using value_type = uint32_t;
    using internal_type = StaticString;
    // comparison operators
    constexpr bool operator==(const Type& other) const { return concreteTypeId == other.concreteTypeId; }
    constexpr std::strong_ordering operator<=>(const Type& other) const { return concreteTypeId <=> other.concreteTypeId; }

    template <class T> requires Reflection::IsReflected<T>
    static constexpr Type GetType() { return Type(GetTypeName<T>()); }

    template <class T> requires !Reflection::IsReflected<T> 
    static constexpr Type GetType() { return Type(GetTypeName<T>()); }
    
    template <class T> requires Reflection::IsReflected<T>
    static constexpr StaticString GetTypeName() { return Reflection::Class<std::remove_const_t<T>>::GetName(); }

    template <class T> requires !Reflection::IsReflected<T>
    static constexpr StaticString GetTypeName() { return StaticString(ctti::nameof<std::remove_const_t<T>>()); }
    
    constexpr StaticString GetName() const {return concreteTypeId; }
    
    template <class T> requires std::is_arithmetic_v<T>
    constexpr operator T() const { return static_cast<T>(concreteTypeId); }

    template <class T>
    constexpr operator T() const {return static_cast<T>(concreteTypeId); }


    template <class T>
    static Array<Type> GetChildren()
    {
        return Reflection::Class<T>::GetChildren();
    }

    template <class T>
    static Ref<T> CreateChild(StaticString name, auto&... args)
    {
        return Reflection::Class<T>::CreateChild(name, args...);
    }

    // static Reference CreateChild(Type type, StaticString name);
    
    constexpr Type() = default;
    constexpr Type(const Type& other) = default;
    constexpr Type& operator=(const Type& other)
    {
        concreteTypeId = other.concreteTypeId;
        return *this;
    }
    
    constexpr Type(StaticString str) : concreteTypeId(str)
#ifdef SC_DEV_VERSION
    , debugName(str.data())
#endif // SC_DEV_VERSION
    {}

    // fields stuff
    template <class T>
    static auto GetFields() { return Reflection::Class<T>::GetFields(); }

    static std::vector<Field> GetFields(Type type);

    template <class T>
    static auto GetField(StaticString name) { return Reflection::Class<T>::GetField(name); }

    Reference GetField(Reference object, std::string_view name) const;
    
    operator StaticString() const { return concreteTypeId; }
private:
    StaticString concreteTypeId = {""_ss};
#ifdef SC_DEV_VERSION
    const std::string_view debugName;
#endif // SC_DEV_VERSION
};

export template <>
struct std::hash<Type>
{
    size_t operator()(const Type& type) const noexcept
    {
        return std::hash<Type::value_type>{}(type);
    }
};

export 
template <class... TTypes>
struct TypeHolder
{
    std::array<Type, sizeof...(TTypes)> types;
    TypeHolder() : types{ (Type::GetType<TTypes>())... } 
    {
    }
};

// Any class that inherits this becomes a FastType enabled class. i.e fast reflection
export class Base
{
    friend Type;

    using Super = Base; 
public :
    virtual ~Base() = default;

    template <class T>
    constexpr const T* Cast() const
    {
        if /*constexpr*/ (IsOfType(Type::GetType<T>()))
        {
            return static_cast<const T*>(this);
        }
        
        return nullptr;
    }

    template <class T>
    constexpr T* Cast()
    {
        if /*constexpr*/ (IsOfType(Type::GetType<T>()))
        {
            return static_cast<T*>(this);
        }
        
        return nullptr;
    }

    template <class T>
    constexpr const T* Cast(Type type) const
    {
        if (IsOfType(type))
            return static_cast<const T*>(this);
        return nullptr;
    }

    template <class T>
    constexpr T* Cast(Type type)
    {
        if (IsOfType(type))
            return static_cast<T*>(this);
        return nullptr;
    }

    template <class T>
    [[nodiscard]] std::unique_ptr<T> Copy() const
    {
        if (ValidateTargetType<T>())
            // No need to use Cast<T>() here since we've already checked the type
            return std::unique_ptr<T>(static_cast<T*>(VirtualCopy()));
        Assert(false);
        return nullptr;
    }

#ifdef EDITOR
    virtual bool DrawEditorInfo() {return false;};
#endif
    
    virtual constexpr Type GetType() const = 0;
    virtual constexpr StaticString GetTypeName() const = 0;
    
    // investigate why fails at compile time
    virtual constexpr bool IsOfType(const Type targetType) const { return false; };
protected:
    
    // Creates a copy to the derived object on the heap and returns a pointer. 
    [[nodiscard]] virtual Base* VirtualCopy() const
    {
        Assert(false);
        return nullptr;
    };
    
    // This templated method is used by Copy overrides since if constexpr clauses are always evaluated if not in a templated function
    // We don't want that because we only want components able to copy to be affected here
    template <class T>
    [[nodiscard]]
    T* InternalCopy() const
    {
        if constexpr (std::is_copy_constructible_v<T>)
        {
            return new T(*this->Cast<T>());
        }
        Assert(false);
        return nullptr;
    }

    template <class T>
    [[nodiscard]]
    T* InternalCopy()
    {
        if constexpr (std::is_copy_constructible_v<T>)
        {
            return new T(*this->Cast<T>());
        }
        Assert(false);
        return nullptr;
    } 
    
private:
    template <class T>
    constexpr bool ValidateTargetType() const
    {
        return IsOfType(Type::GetType<T>());
    }
};