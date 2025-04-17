module;

#include <d3d11.h>

export module Graphics:InputLayout;

import Common;
import :Device;

template<class T>
struct TraitInputElement;

using InputLayoutSemantics = int;

template <InputLayoutSemantics T>
struct TraitSemanticToType;

template <InputLayoutSemantics T>
struct TraitSemanticInstanced;

template <InputLayoutSemantics T>
struct ShaderInputAttribute;

template <>
struct TraitInputElement<Vector2>
{
    static constexpr auto value = 1;
    static constexpr auto format = DXGI_FORMAT_R32G32_FLOAT;
};

template <>
struct TraitInputElement<Vector3>
{
    static constexpr auto value = 1;
    static constexpr auto format = DXGI_FORMAT_R32G32B32_FLOAT;
};

template <>
struct TraitInputElement<Vector4>
{
    static constexpr auto value = 1;
    static constexpr auto format = DXGI_FORMAT_R32G32B32A32_FLOAT;
};

template <>
struct TraitInputElement<DirectX::XMFLOAT3X3>
{
    static constexpr auto value = 3;
    static constexpr auto format = DXGI_FORMAT_R32G32B32_FLOAT;
};

template <>
struct TraitInputElement<Matrix>
{
    static constexpr auto value = 4;
    static constexpr auto format = DXGI_FORMAT_R32G32B32A32_FLOAT;
};

template <>
struct TraitInputElement<Color>
{
    static constexpr auto value = 4;
    static constexpr auto format = DXGI_FORMAT_R32G32B32A32_FLOAT;
};

template <>
struct TraitInputElement<float>
{
    static constexpr auto value = 1;
    static constexpr auto format = DXGI_FORMAT_R32_FLOAT;
};

constexpr D3D11_INPUT_ELEMENT_DESC CreateElement(const char* name, UINT index, DXGI_FORMAT format, bool instanced)
{
    return {
        name,
        index,
        format,
        instanced ? 1u : 0u,
        D3D11_APPEND_ALIGNED_ELEMENT,
        instanced ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA,
        instanced ? 1u : 0u
    };
}

template <class T>
constexpr auto CreateElementDescription(
    const char* name, bool instanced = false)
{
    std::array<D3D11_INPUT_ELEMENT_DESC, TraitInputElement<T>::value> elements;
    for (int i = 0; i < elements.size(); ++i)
        elements[i] = CreateElement(name, i, TraitInputElement<T>::format, instanced);
    return elements;
}

template<std::size_t ... Tn>
constexpr int sum()
{
    int total = 0;
    ((total += Tn), ...);
    return total;
}

template <std::size_t ...Sizes>
constexpr std::array<D3D11_INPUT_ELEMENT_DESC, sum<Sizes...>()> concat(
    std::array<D3D11_INPUT_ELEMENT_DESC, Sizes>... rest)
{
    std::array<D3D11_INPUT_ELEMENT_DESC, sum<Sizes...>()> result{};
    std::size_t index = 0;

    ([&](auto&& elems) {
        for (auto&& el : elems) {
            result[index] = std::move(el);
            ++index;
        }
    }(rest), ...);

    return result;
}

template <InputLayoutSemantics T, bool Instanced>
struct ShaderInputAttributeBase
{
    using InnerType = typename TraitSemanticToType<T>::type;
    static constexpr auto descriptor = CreateElementDescription<InnerType>(TraitSemanticToType<T>::name, Instanced);
};

template <class, class>
struct Cons;

template <class T, class... Args>
struct Cons<T, std::tuple<Args...>>
{
    using type = std::tuple<T, Args...>;
};

template <template <class> class Pred, class...>
struct filter;

template <template <class> class Pred>
struct filter<Pred>
{
    using type = std::tuple<>;
};

template <template <class> class Pred, class Head, class... Tail>
struct filter<Pred, Head, Tail...>
{
    using type = std::conditional_t<
        Pred<Head>::value,
        typename Cons<Head, typename filter<Pred, Tail...>::type>::type,
        typename filter<Pred, Tail...>::type
    >;
};

template <class... T>
auto tupleToStruct(std::tuple<T...>)
{
    struct _ : T... { } t; return t;
}

template <class T>
struct PredicatePerVertex
{
    static constexpr auto value = !T::instanced;
};

template <class T>
struct PredicatePerInstanced
{
    static constexpr auto value = T::instanced;
};

export struct InputLayoutDescription
{
    virtual ~InputLayoutDescription() = default;
    virtual const D3D11_INPUT_ELEMENT_DESC* GetInputElementDesc() const = 0;
    virtual UINT GetElementCount() const = 0;
};

export template <InputLayoutSemantics... Ts>
struct InputLayoutBuilder : InputLayoutDescription
{
    static constexpr auto inputLayout = concat(ShaderInputAttribute<Ts>::descriptor...);

    struct VertexType : decltype(tupleToStruct(typename filter<PredicatePerVertex, ShaderInputAttribute<Ts>...>::type{}))
    { };

    struct InstanceType : decltype(tupleToStruct(typename filter<PredicatePerInstanced, ShaderInputAttribute<Ts>...>::type{}))
    { };

    const D3D11_INPUT_ELEMENT_DESC* GetInputElementDesc() const override
    {
        return inputLayout.data();
    }

    UINT GetElementCount() const override
    {
        return static_cast<UINT>(inputLayout.size());
    }
};

#define DECLARE_SEMANTIC_TYPE(Type, Name, Semantic, Instanced)                                                         \
    export enum                                                                                                        \
    {                                                                                                                  \
        Semantic = __LINE__                                                                                            \
    };                                                                                                                 \
    template <>                                                                                                        \
    struct TraitSemanticToType<Semantic>                                                                               \
    {                                                                                                                  \
        using type = Type;                                                                                             \
        static constexpr const char* name = #Semantic;                                                                 \
    };                                                                                                                 \
    template <>                                                                                                        \
    struct ShaderInputAttribute<Semantic> : ShaderInputAttributeBase<Semantic, Instanced>                              \
    {                                                                                                                  \
        InnerType Name;                                                                                                \
        static constexpr bool instanced = Instanced;                                                                   \
    };

#define DECLARE_SEMANTIC_TYPE_VERTEX(Type, Name, Semantic) DECLARE_SEMANTIC_TYPE(Type, Name, Semantic, false)
#define DECLARE_SEMANTIC_TYPE_INSTANCE(Type, Name, Semantic) DECLARE_SEMANTIC_TYPE(Type, Name, Semantic, true)

DECLARE_SEMANTIC_TYPE_VERTEX(Vector3, position, POSITION);
DECLARE_SEMANTIC_TYPE_VERTEX(Color, color, COLOR);
DECLARE_SEMANTIC_TYPE_VERTEX(Vector3, normal, NORMAL);
DECLARE_SEMANTIC_TYPE_VERTEX(Vector2, texCoord, TEXCOORD);

DECLARE_SEMANTIC_TYPE_INSTANCE(Matrix, modelMatrix, MODEL_MATRIX);

using GenericInputLayout = InputLayoutBuilder<POSITION, NORMAL, TEXCOORD>;
export using GenericVertex = GenericInputLayout::VertexType;
export using GenericInstance = GenericInputLayout::InstanceType;

using DebugLinesInputLayout = InputLayoutBuilder<POSITION, COLOR>;
export using DebugLinesVertex = DebugLinesInputLayout::VertexType;

namespace InputLayouts
{
    export constexpr DebugLinesInputLayout DebugLines;
    export constexpr GenericInputLayout Default;
}