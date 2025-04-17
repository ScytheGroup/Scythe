export module Reflection:Reference;
import Common;
import :Core;
import :Type;

/**
 * @brief Wrapper to old a ref to a type and allows type validation using an id. 
 **/
export class Reference
{
    Type type;
    void* ref = nullptr;

public:
    Reference() = default;
    Reference(Reference& ref) = default;

    template <class T>
    Reference(T& ref);

    template <class T>
    T* Cast();

    std::optional<int64_t> GetIntegral();
    std::optional<double> GetFloating();

    // will validate the type of the reference if it is known
    template <class T>
    bool IsOfType();

    template <class T>
    T* GetField(StaticString name);

    bool IsNull() const;
};

template <class T>
Reference::Reference(T& ref):
    type(Type::GetType<T>()), ref(const_cast<std::remove_const_t<T>*>(&ref))
{
}

template <class T>
T* Reference::Cast()
{
    if (IsOfType<std::remove_const_t<T>>())
    {
        return static_cast<T*>(ref);
    }

    return nullptr;
}
export {
// Helper function allows to get the ref value as an integral type instead of a reference
std::optional<int64_t> Reference::GetIntegral()
{
    switch (type.GetName().hash())
    {
    case Type::GetType<int8_t>():
        return *static_cast<int8_t*>(ref);
    case Type::GetType<int16_t>():
        return *static_cast<int16_t*>(ref);
    case Type::GetType<int32_t>():
        return *static_cast<int32_t*>(ref);
    case Type::GetType<int64_t>():
        return *static_cast<int64_t*>(ref);
    case Type::GetType<uint8_t>():
        return *static_cast<uint8_t*>(ref);
    case Type::GetType<uint16_t>():
        return *static_cast<uint16_t*>(ref);
    case Type::GetType<uint32_t>():
        return *static_cast<uint32_t*>(ref);
    case Type::GetType<uint64_t>():
        return *static_cast<uint64_t*>(ref);
    default:
        return {};
    }
}

// Helper function allows to get the ref value as a floating point type instead of a reference
std::optional<double> Reference::GetFloating()
{
    switch (type.GetName().hash())
    {
    case Type::GetType<float>():
        return *static_cast<float*>(ref);
    case Type::GetType<double>():
        return *static_cast<double*>(ref);
    default:
        return {};
    }
}

template <class T>
bool Reference::IsOfType()
{
    std::string currentType = type.GetName();
    std::string target = Type::GetType<T>().GetName();
    bool isSame = type == Type::GetType<T>();
    return isSame;
}

template <class T>
T* Reference::GetField(StaticString name)
{
    return type.GetField<T>(ref, name);
}

bool Reference::IsNull() const
{
    return ref != nullptr;
}
}
export namespace Reflection
{
    template <>
    class Class<Reference> : public IClass
    {
    public:
        static consteval StaticString GetName() { return "Reference"_ss; }
    };
}
