export module Common:TypeFormats;

import :Math;

import <format>;

template <>
struct std::formatter<Vector4> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const Vector4& obj, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{} {} {} {}", obj.x, obj.y, obj.z, obj.w);
    }
};

template <>
struct std::formatter<Vector3> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }
    
    auto format(const Vector3& obj, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{} {} {}", obj.x, obj.y, obj.z);
    }
};

template <>
struct std::formatter<Vector2> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const Vector2& obj, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{} {}", obj.x, obj.y);
    }
};

template <>
struct std::formatter<DirectX::XMUINT3> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const DirectX::XMUINT3& obj, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{} {} {}", obj.x, obj.y, obj.z);
    }
};

template <>
struct std::formatter<DirectX::XMINT3> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const DirectX::XMINT3& obj, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{} {} {}", obj.x, obj.y, obj.z);
    }
};

template <>
struct std::formatter<DirectX::XMUINT2> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const DirectX::XMUINT2& obj, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{} {}", obj.x, obj.y);
    }
};

template <>
struct std::formatter<DirectX::XMINT2> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const DirectX::XMINT2& obj, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{} {}", obj.x, obj.y);
    }
};
