module;

#include <cassert>

export module Common:ArchiveStream;

import std;

template<class ...Ts>
struct Visitor : Ts... {
    using Ts::operator()...;
};

export
{
    class ArchiveStream
    {
        const bool isWriting; 
        std::variant<std::ifstream, std::ofstream> file;
    public:
        struct Out{};
        struct In{};
        
        ArchiveStream(const std::filesystem::path& filePath, Out)
            : file{ std::ofstream{ filePath, std::ios::binary } }
            , isWriting{ true }
        {
        }
        
        ArchiveStream(const std::filesystem::path& filePath, In)
            : file{ std::ifstream{ filePath, std::ios::binary } }
            , isWriting{ false }
        {
        }

        bool IsReading() { return !isWriting; }
        bool IsWriting() { return isWriting; }

        void Skip(int byteCount)
        {
            std::visit(Visitor{
                [&](std::ifstream& is)
                {
                    is.ignore(byteCount);
                },
                [](auto&) {}
            }, file);
        }

        template<class T> requires std::is_trivial_v<T>
        friend ArchiveStream& operator<<(ArchiveStream& os, const T& a)
        {
            std::visit(Visitor{
                [&](std::ofstream& os) {
                    os.write(reinterpret_cast<const char*>(&a), sizeof(T));    
                },
                [](auto&) {
                    assert(false);
                }
            }, os.file);
            return os;
        }
        
        template<class T> requires std::is_trivial_v<T>
        friend ArchiveStream& operator>>(ArchiveStream& os, T& a)
        {
            std::visit(Visitor{
                [&](std::ifstream& is) {
                    is.read(reinterpret_cast<char*>(&a), sizeof(T));
                },
                [](auto&) {
                    assert(false);
                }
            }, os.file);
            return os;
        }
    };

    template<class T>
    ArchiveStream& operator<<(ArchiveStream& os, const std::vector<T>& a)
    {
        size_t size = a.size();
        os << size;
        for (auto& elem : a)
            os << elem;
        return os;
    }

    template<class T>
    ArchiveStream& operator>>(ArchiveStream& os, std::vector<T>& a)
    {
        size_t size;
        os >> size;
        a.reserve(size);
        for (size_t i = 0; i < size; i++)
        {
            T c; os >> c;
            a.push_back(c);
        }
        return os;
    }

    template<class T1, class T2>
    ArchiveStream& operator<<(ArchiveStream& os, const std::pair<T1, T2>& a)
    {
        os << a.first;
        os << a.second;
        return os;
    }

    template<class T1, class T2>
    ArchiveStream& operator>>(ArchiveStream& os, std::pair<T1, T2>& a)
    {
        os >> a.first;
        os >> a.second;
        return os;
    }

    template<class K, class V>
    ArchiveStream& operator<<(ArchiveStream& os, const std::unordered_map<K, V>& a)
    {
        size_t size = a.size();
        os << size;
        for (auto& elem : a)
            os << elem;
        return os;
    }
    
    template<class K, class V>
    ArchiveStream& operator>>(ArchiveStream& os, std::unordered_map<K, V>& a)
    {
        size_t size;
        os >> size;
        a.reserve(size);
        for (size_t i = 0; i < size; i++)
        {
            std::pair<K, V> p;
            os >> p;
            a.push_back(p);
        }
        return os;
    }

    template<class T> requires std::is_convertible_v<T, std::string> 
    ArchiveStream& operator<<(ArchiveStream& os, const T& a)
    {
        std::string s = static_cast<std::string>(a);
        size_t size = s.size();
        os << size;
        for (auto& elem : s)
            os << elem;
        return os;
    }

    template<class T> requires std::is_constructible_v<T, std::string> 
    ArchiveStream& operator>>(ArchiveStream& os, T& a)
    {
        size_t size;
        os >> size;
        std::string val;
        val.reserve(size);
        for (size_t i = 0; i < size; i++)
        {
            char c; os >> c;
            val.push_back(c);
        }
        a = T{std::move(val)};
        return os;
    }

    template<class T>
    ArchiveStream& operator<<(ArchiveStream& os, const std::optional<T>& a)
    {
        bool valid = a.has_value();
        os << valid;
        if (valid)
            os << *a;
        return os;
    }

    template<class T>
    ArchiveStream& operator>>(ArchiveStream& os, std::optional<T>& a)
    {
        bool valid;
        os >> valid;
        if (valid)
        {
            a = T{};
            os >> *a;
        }
        return os;
    }

    template<class T>
    ArchiveStream& operator<<(ArchiveStream& os, const T& a);
    
    template<class T> 
    ArchiveStream& operator>>(ArchiveStream& os, T& a);
}

