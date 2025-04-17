export module Common:Handle;

import <cstdint>;
import <limits>;
import <format>;
import <type_traits>;
import <compare>;

// Generic handle class that is an id and could
// probably take a pointer to representing entity for quick access
export
struct Handle
{
    template <class T>
    requires std::is_integral_v<T>
    constexpr operator T() const noexcept
    {
        return static_cast<T>(id);
    }

    Handle() : id{std::numeric_limits<value_type>::max()} {};
    
    template <class T>
    requires std::is_arithmetic_v<T>
    Handle(T val) : id{static_cast<uint32_t>(val)} {}

    auto operator<=>(const Handle&) const;
    template <class T>
    requires std::is_integral_v<T>
    auto operator<=>(const T) const;
        
    bool operator==(const Handle& other) const
    {
        return id == other.id;
    }
    
    using value_type = uint32_t;
    value_type id;

    template <class T>
    requires std::is_arithmetic_v<T>
    void operator=(T number)
    {
        id = static_cast<uint32_t>(number);
    }

    Handle(const Handle& ) = default;
    Handle& operator=(const Handle& number) = default;
    Handle(Handle&& ) = default;
    Handle& operator=(Handle&& number) = default;

    Handle& operator++(int)
    {
        ++id;
        return *this;
    }

    Handle operator++()
    {
        auto save = *this;
        ++id;
        return save;
    }

    bool IsValid() const
    {
        return id != std::numeric_limits<value_type>::max();
    }

    void Invalidate()
    {
        id = std::numeric_limits<value_type>::max();
    }
};

template <class T> requires std::is_integral_v<T>
auto Handle::operator<=>(const T val) const
{
    return id <=> static_cast<uint32_t>(val);
}

auto Handle::operator<=>(const Handle& other) const
{
    return id <=> other.id;
}

template <>
struct std::formatter<Handle> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const Handle& id, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "Handle({})", static_cast<unsigned>(id));
    }
};

template<>
struct std::hash<Handle>
{
    std::size_t operator()(const Handle& k) const
    {
        return std::hash<Handle::value_type>()(k.id);
    }
};
