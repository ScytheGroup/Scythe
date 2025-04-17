export module Common:Function;
import <functional>;

export template <class T, class TRet, class... TArgs>
constexpr std::function<TRet(TArgs...)> BindMethod(TRet (T::* callback) (TArgs...), T* ptr)
{
    if constexpr (sizeof...(TArgs) == 0)
    {
        return std::bind(callback, ptr);
    }
    else if constexpr (sizeof...(TArgs) == 1)
    {
        return std::bind(callback, ptr, std::placeholders::_1);
    }
    else if constexpr (sizeof...(TArgs) == 2)
    {
        return std::bind(callback, ptr, std::placeholders::_1, std::placeholders::_2);
    }
    else if constexpr (sizeof...(TArgs) == 3)
    {
        return std::bind(callback, ptr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    }
    // If support needed for more, you can add it on top of this comment.
    else
    {
        static_assert(sizeof...(TArgs) <= 3, "BindMethod does not support this number of arguments.");
    }
    
    std::unreachable();
}

export template<class... Ts>
struct Visitor : Ts... { using Ts::operator()...; };
