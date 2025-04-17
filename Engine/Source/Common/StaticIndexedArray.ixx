module;

#include <cassert>

export module StaticIndexedArray;

import std;

// The elements of this class will stay at the same index in the container during their existence
export template<class T>
class StaticIndexedArray
{
    std::vector<std::optional<T>> elements;
    std::deque<size_t> freeList;

    bool IsFree(size_t i) const;
    bool IsValidIndex(size_t i) const;
    void RemoveUselessFreeSlots();
    size_t GetFirstElementIndex() const;
public:
    size_t Add(T&&);
    size_t Add(const T&);
    void Remove(size_t i);
    size_t Size() const;
    bool Empty() const;
    void Clear();
    void Reserve(size_t size);
    template <class ... TArgs>
    size_t Emplace(TArgs... args);
    bool HasValue(size_t i) const
    {
        return IsValidIndex(i);
    }
    
    T& operator[](size_t i);
    const T& at(size_t i) const;
    T& at(int i);

    class Iterator
    {
        using VecIt = std::optional<T>*; 
        VecIt it, lowerBound, upperBound, first;
        Iterator(VecIt it, VecIt lowerBound, VecIt upperBound, VecIt first);
    public:
        T& operator*();
        Iterator operator++();
        Iterator operator++(int);
        Iterator operator--();
        Iterator operator--(int);

        size_t AbsoluteIndex()
        {
            return static_cast<size_t>(std::distance(first, it));
        }

        friend bool operator==(const Iterator& a, const Iterator& b)
        {
            return a.it == b.it;
        }
        
        T* operator->()
        {
            return &**it;
        }
        
        friend StaticIndexedArray;
    };

    class ConstIterator
    {
        using VecIt = const std::optional<T>*; 
        VecIt it, lowerBound, upperBound, first;
        ConstIterator(VecIt it, VecIt lowerBound, VecIt upperBound, VecIt first);
    public:
        const T& operator*() const;
        ConstIterator operator++();
        ConstIterator operator++(int);
        ConstIterator operator--();
        ConstIterator operator--(int);

        friend bool operator==(const ConstIterator& a, const ConstIterator& b)
        {
            return a.it == b.it;
        }

        T* operator->()
        {
            return &**it;
        }
        
        std::uint32_t AbsoluteIndex()
        {
            return std::distance(first, it);
        }
        
        friend StaticIndexedArray;
    };

    Iterator begin();
    Iterator end();
    ConstIterator begin() const;
    ConstIterator end() const;

};

template <class T>
bool StaticIndexedArray<T>::IsFree(size_t i) const
{
    return !elements[i].has_value();
}

template <class T>
bool StaticIndexedArray<T>::IsValidIndex(size_t i) const
{
    return i < elements.size() && !IsFree(i);
}

template <class T>
void StaticIndexedArray<T>::RemoveUselessFreeSlots()
{
    auto rIt = elements.rbegin();
    while (rIt != elements.rend() && !rIt->has_value())
        ++rIt;

    size_t nbItemsToRemove = std::distance(elements.rbegin(), rIt);
    
    freeList.resize(freeList.size() - nbItemsToRemove);
    elements.resize(elements.size() - nbItemsToRemove);
}

template <class T>
size_t StaticIndexedArray<T>::GetFirstElementIndex() const
{
    int current = 0;
    while (!freeList.empty() && current < freeList.size() && freeList[current] == current)
        current++;
    return current;
}

template <class T>
size_t StaticIndexedArray<T>::Add(T&& element)
{
    size_t index;
    if (freeList.empty())
    {
        elements.push_back(std::move(element));
        index = elements.size() - 1;
    }
    else
    {
        index = freeList.front();
        elements[index] = std::move(element);
        freeList.pop_front();
    }
    return index;
}

template <class T>
size_t StaticIndexedArray<T>::Add(const T& element)
{
    size_t index;
    if (freeList.empty())
    {
        elements.push_back(element);
        index = elements.size() - 1;
    }
    else
    {
        index = freeList.front();
        elements[index] = element;
        freeList.pop_front();
    }
    return index;
}

template <class T>
void StaticIndexedArray<T>::Remove(size_t i)
{
    if (!IsValidIndex(i))
        throw std::out_of_range{"No element at that index"};

    elements[i] = {};
    freeList.insert(std::ranges::lower_bound(freeList, i), i);
    
    if (i == elements.size() - 1)
    {
        RemoveUselessFreeSlots();
    }
}

template <class T>
size_t StaticIndexedArray<T>::Size() const
{
    return elements.size() - freeList.size();
}

template <class T>
bool StaticIndexedArray<T>::Empty() const
{
    return Size() == 0;
}

template <class T>
void StaticIndexedArray<T>::Clear()
{
    elements.clear();
    freeList.clear();
}

template <class T>
void StaticIndexedArray<T>::Reserve(size_t size)
{
    elements.reserve(size);
}

template <class T>
template <class... TArgs>
size_t StaticIndexedArray<T>::Emplace(TArgs... args)
{
    return Add(T{std::forward<TArgs>(args)...});
}

template <class T>
T& StaticIndexedArray<T>::operator[](size_t i)
{
    return *elements[i];
}

template <class T>
const T& StaticIndexedArray<T>::at(size_t i) const
{
    assert(!IsValidIndex(i));
    return *elements[i];
}

template <class T>
T& StaticIndexedArray<T>::at(int i)
{
    assert(!IsValidIndex(i));
    return *elements[i];
}

template <class T>
StaticIndexedArray<T>::Iterator::Iterator(VecIt it, VecIt lowerBound, VecIt upperBound, VecIt first)
    : it{it}
    , lowerBound{lowerBound}
    , upperBound{upperBound}
    , first{first}
{
}

template <class T>
StaticIndexedArray<T>::ConstIterator::ConstIterator(VecIt it, VecIt lowerBound, VecIt upperBound, VecIt first)
    : it{it}
    , lowerBound{lowerBound}
    , upperBound{upperBound}
    , first{first}
{
}

template <class T>
T& StaticIndexedArray<T>::Iterator::operator*()
{
    return **it;
}

template <class T>
typename StaticIndexedArray<T>::Iterator StaticIndexedArray<T>::Iterator::operator++()
{
    do ++it; while(it != upperBound && !it->has_value());
    return *this;
}

template <class T>
typename StaticIndexedArray<T>::Iterator StaticIndexedArray<T>::Iterator::operator++(int)
{
    auto prev = *this;
    do ++it; while(it != upperBound && !it->has_value());
    return prev;
}

template <class T>
typename StaticIndexedArray<T>::Iterator StaticIndexedArray<T>::Iterator::operator--()
{
    do --it; while(it != lowerBound && !it->has_value());
    return *this;
}

template <class T>
typename StaticIndexedArray<T>::Iterator StaticIndexedArray<T>::Iterator::operator--(int)
{
    auto prev = *this;
    do --it; while(it != lowerBound && !it->has_value());
    return prev;
}

template <class T>
const T& StaticIndexedArray<T>::ConstIterator::operator*() const
{
    return **it;
}

template <class T>
typename StaticIndexedArray<T>::ConstIterator StaticIndexedArray<T>::ConstIterator::operator++()
{
    do ++it; while(it != upperBound && !it->has_value());
    return *this;
}

template <class T>
typename StaticIndexedArray<T>::ConstIterator StaticIndexedArray<T>::ConstIterator::operator++(int)
{
    auto prev = *this;
    do ++it; while(it != upperBound && !it->has_value());
    return prev;
}

template <class T>
typename StaticIndexedArray<T>::ConstIterator StaticIndexedArray<T>::ConstIterator::operator--()
{
    do --it; while(it != lowerBound && !it->has_value());
    return *this;
}

template <class T>
typename StaticIndexedArray<T>::ConstIterator StaticIndexedArray<T>::ConstIterator::operator--(int)
{
    auto prev = *this;
    do --it; while(it != lowerBound && !it->has_value());
    return prev;
}

template <class T>
typename StaticIndexedArray<T>::Iterator StaticIndexedArray<T>::begin()
{
    int current = 0;
    while (!freeList.empty() && current < freeList.size() && freeList[current] == current)
        current++;
    
    return Iterator{
        elements.data() + current, 
        elements.data() + current - 1,
        elements.data() + std::max(elements.size() - 1, 0ULL) + 1,
        elements.data()
    };
}

template <class T>
typename StaticIndexedArray<T>::Iterator StaticIndexedArray<T>::end()
{
    auto it = elements.data() + std::max(elements.size() - 1, 0ULL) + 1;
    return Iterator{
        it,
        begin().lowerBound,
        it,
        elements.data()
    };
}

template <class T>
typename StaticIndexedArray<T>::ConstIterator StaticIndexedArray<T>::begin() const
{
    int current = 0;
    while (!freeList.empty() && current < freeList.size() && freeList[current] == current)
        current++;
    
    return ConstIterator{
        elements.data() + current, 
        elements.data() + current - 1,
        elements.data() + std::max(elements.size() - 1, 0ULL) + 1,
        elements.data()
    };
}

template <class T>
typename StaticIndexedArray<T>::ConstIterator StaticIndexedArray<T>::end() const
{
    const auto beforeBegin = elements.data() + GetFirstElementIndex();
    const auto* endPtr = elements.data() + std::max(elements.size() - 1, 0ULL) + 1;
    return ConstIterator{
        endPtr,
        beforeBegin - 1,
        endPtr,
        elements.begin()
    };
}
