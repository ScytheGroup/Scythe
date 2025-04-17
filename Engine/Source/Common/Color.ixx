export module Common:Color;

import :Math;
import <format>;

export namespace Colors
{
    constexpr Color Black{ 0, 0, 0 };
    constexpr Color White{ 1, 1, 1 };
    constexpr Color Red{ 1, 0, 0 };
    constexpr Color Green{ 0, 1, 0 };
    constexpr Color Blue{ 0, 0, 1 };
    constexpr Color Cyan{ 0, 1, 1 };
    constexpr Color Purple{ 1, 0, 1 };
    constexpr Color Yellow{ 1, 1, 0 };
}

template<>
struct std::formatter<Color>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const Color& color, FormatContext& ctx) const
    {
        return format_to(ctx.out(), "R:{} G:{} B:{} A:{}", color.x, color.y, color.z, color.w);
    }
};