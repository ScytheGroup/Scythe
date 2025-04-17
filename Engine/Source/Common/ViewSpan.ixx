export module Common:ViewSpan;

import <cstdint>;
import <iterator>;

import :Debug;

export template <typename T>
struct ViewSpan
{
    ViewSpan(T* data, uint32_t count = 1)
        : data{ data }
        , count{ count }
    {}

    ViewSpan(const T* data, uint32_t count = 1)
        : data{ const_cast<T*>(data) }
        , count{ count }
    {}
    
    template <class InputIt> requires std::contiguous_iterator<InputIt>
    ViewSpan(InputIt first, InputIt last)
        : data{ &*first }
        , count{ static_cast<uint32_t>(std::distance(first, last)) }
    {}

    ViewSpan(ViewSpan&&) = default;
    ViewSpan& operator=(ViewSpan&&) = default;
    
    T* data;
    uint32_t count;

    T* begin() { return data; }
    const T* begin() const { return data; }

    T* end() { return data + count; }
    const T* end() const { return data + count; }

    T& operator[](int i);
    T& at(int i);
    const T& at(int i) const;
};

template <typename T>
T& ViewSpan<T>::operator[](int i)
{
    return data[i];
}

template <typename T>
T& ViewSpan<T>::at(int i)
{
    Assert(i >= 0 && i < count);
    return data[i]; 
}

template <typename T>
const T& ViewSpan<T>::at(int i) const
{
    Assert(i >= 0 && i < count);
    return data[i]; 
}
