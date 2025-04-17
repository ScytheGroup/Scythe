export module Common:SH;
import <array>;
import <algorithm>;

import :Math;

export using ScalarSH9 = std::array<float, 9>;

export inline ScalarSH9 MakeSH9(Vector3& normal);

export ScalarSH9 operator*(const ScalarSH9& a, const ScalarSH9& b)
{
    using namespace std;
    ScalarSH9 ret;
    transform(begin(a), end(a), begin(b), begin(ret), multiplies{});
    return ret;
};

export ScalarSH9 operator*(const ScalarSH9& sh, float val)
{
    using namespace std;
    ScalarSH9 ret;
    ranges::transform(sh, begin(ret), [val](float f)
    {
        return f * val;
    });
    return ret;
}

export ScalarSH9 operator+(const ScalarSH9& a, const ScalarSH9& b)
{
    using namespace std;
    ScalarSH9 ret;
    transform(begin(a), end(a), begin(b), begin(ret), plus{});
    return ret;
}

export struct ColorSH9
{
    ScalarSH9 r{};
    ScalarSH9 g{};
    ScalarSH9 b{};

    ColorSH9 operator+(const ColorSH9& other) const noexcept
    {
        ColorSH9 ret;
        ret.r = r + other.r;
        ret.g = g + other.g;
        ret.b = b + other.b;
        return ret;
    }

    ColorSH9& operator+=(const ColorSH9& other)
    {
        *this = *this + other;
        return *this;
    }

    ColorSH9 operator*(const ColorSH9& other) const noexcept
    {
        ColorSH9 ret;
        ret.r = r * other.r;
        ret.g = g * other.g;
        ret.b = b * other.b;
        return ret;
    }

    ColorSH9 operator*(float val) const noexcept
    {
        ColorSH9 ret;
        ret.r = r * val;
        ret.g = g * val;
        ret.b = b * val;
        return ret;
    }

    ColorSH9& operator*=(float val)
    {
        *this = *this * val;
        return *this;
    }

    void Reduce();
    void ApplyClampedCosineLobe();
};

export inline ColorSH9 ProjectSH9(Vector3 normal, const Color& color);