module Common;

import :Guid;

Guid Guid::Create()
{
    static std::random_device dev;
    static std::mt19937 rng(dev());

    std::uniform_int_distribution<int> dist(0, 15);

    const char *v = "0123456789abcdef";
    const bool dash[] = { 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0 };

    std::string res;
    for (int i = 0; i < 16; i++) {
        if (dash[i]) res += "-";
        res += v[dist(rng)];
        res += v[dist(rng)];
    }
    
    return Guid(res);
}

bool Guid::operator==(const Guid& other) const
{
    return std::string_view(data) == std::string_view(other.data);
}

bool Guid::IsValid() const noexcept
{
    return std::accumulate(std::begin(data), std::end(data), 0) > 0;
}

std::string Guid::ToString() const
{
    return std::string(data);
}

Guid::operator std::string() const
{
    return std::string(data);
}

Guid::Guid(const std::string& strGuid)
    : Guid(std::string_view{strGuid})
{
}

Guid::Guid(const std::string_view& strGuid)
{
    std::ranges::copy_n(strGuid.begin(), std::min(MaxIdSize, static_cast<int>(strGuid.size())), data);
}

Guid::Guid(const char* strGuid)
    : Guid(std::string_view{strGuid})
{
}

