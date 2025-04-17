module Reflection:Core;
import :Core;

namespace Reflection
{
    std::unordered_map<uint32_t, std::unique_ptr<IClass>>& Internal::GetClasses()
    {
        static std::unordered_map<uint32_t, std::unique_ptr<IClass>> classes;
        return classes;
    }
}
