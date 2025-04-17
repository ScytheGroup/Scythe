export module Common:Array;

import <algorithm>;
import <mutex>;
import <vector>;

// Container class extending vector with additional behaviour
// Right now missing thread-safety semantics
export template <class T, class Allocator = std::allocator<T>>
class Array : public std::vector<T, Allocator>
{
    // Todo: Revisit thread safety
    using Super = std::vector<T, Allocator>;
public:
    Array() = default;
    Array(auto&& ... params) : Super(std::forward<decltype(params)>(params)...){}
    Array(std::initializer_list<T> list) : Super(list) {}
public:
    using iterator = Super::iterator;
    using reverse_iterator = Super::reverse_iterator;
    using value_type = Super::value_type;

    // Swaps and removes. This will break the current order 
    size_t fast_remove(uint32_t index);
    // Swaps and removes. This will break the current order 
    int fast_remove(iterator it);
    int fast_remove(reverse_iterator it);
    // Will find an element, swap and remove. This will break the current order
    int find_remove(T* value);
    template <class TPred>
    int remove_if(TPred pred);

    void push_back_unique(value_type&& val);
    void push_back_unique(const value_type& val);
    
    using Super::operator[];
    using Super::operator=;

    // begin and end are not thread safe
    using Super::begin;
    using Super::end;
    using Super::back;
    using Super::front;
    using Super::rbegin;
    using Super::rend;
    using Super::push_back;

    // move constructor
    Array(Array&& other) noexcept
        : Super(std::move(other))
    {
    }

    // move assignment
    Array& operator=(Array&& other) noexcept
    {
        if (this != &other)
        {
            Super::operator=(std::move(other));
        }
        return *this;
    }

    // copy constructor
    Array(const Array& other)
        : Super(other)
    {
    }
};

template <class T, class Allocator>
size_t Array<T, Allocator>::fast_remove(uint32_t index)
{
    if (index != Super::size() - 1)
        std::iter_swap(begin() + index, end() - 1);
    Super::pop_back();

    return index;
}

template <class T, class Allocator>
int Array<T, Allocator>::fast_remove(iterator it)
{
    auto distance = std::distance(begin(), it);
    
    if (it != end())
    {
        std::iter_swap(it, end() - 1);
        Super::pop_back();
    }
    // Return the old index of the element that was erased
    return distance;
}

template <class T, class Allocator>
int Array<T, Allocator>::fast_remove(reverse_iterator it)
{
    if (it != end())
    {
        std::iter_swap(it, end() - 1);
        Super::pop_back();
    }
    // Return the old index of the element that was erased
    return std::distance(begin(), it);
}

template <class T, class Allocator>
int Array<T, Allocator>::find_remove(T* value)
{

    auto it = std::find(begin(), end(), value);
    if (it != end())
    {
        std::iter_swap(it, end() - 1);
        Super::pop_back();
    }
    // Return the old index of the element that was erased
    return std::distance(begin(), it);
}

template <class T, class Allocator>
template <class TPred>
int Array<T, Allocator>::remove_if(TPred pred)
{
    auto it = std::remove_if(begin(), end(), pred);
    int count = std::distance(it, end());
    Super::erase(it, end());

    return count;
}

template <class T, class Allocator>
void Array<T, Allocator>::push_back_unique(value_type&& val)
{
    if (std::ranges::find(*this, val) != end())
        return;

    push_back(std::move(val));
}

template <class T, class Allocator>
void Array<T, Allocator>::push_back_unique(const value_type& val)
{
    if (std::ranges::find(*this, val) == end())
        return;

    push_back(val);
}
