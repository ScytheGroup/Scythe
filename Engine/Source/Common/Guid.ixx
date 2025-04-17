module;

#include <random>

export module Common:Guid;

import <string>;
import <format>;
import <utility>;

export class Guid
{
    static constexpr int MaxIdSize = 36;
    char data[MaxIdSize + 1] {};
public:
    static Guid Create();

    Guid() = default;
    Guid(const Guid& guid) = default;
    Guid& operator=(const Guid& guid) = default;
    Guid(Guid&& guid) = default;
    Guid& operator=(Guid&& guid) = default;

    bool operator==(const Guid& other) const;

    bool IsValid() const noexcept;
    std::string ToString() const;
    operator std::string() const;
    
    Guid(const std::string& strGuid);
    Guid(const std::string_view& strGuid);
    Guid(const char* strGuid);
};

template <>
struct std::formatter<Guid> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }
    
    auto format(const Guid& obj, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}", obj.ToString());
    }
};

template <>
struct std::hash<Guid>
{
    size_t operator()(const Guid& guid) const noexcept
    {
        return std::hash<std::string>{}(guid.ToString());
    }
};
