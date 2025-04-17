export module Common:RefCounted;
import :Debug;
import <memory>;

// A non-atomic ref counted object
export template <class T>
class Ref
{
    T* ptr = nullptr;
    using count_t = uint32_t;
    count_t* refCount = nullptr;
    friend Ref;
    
    inline void DecrementRefCount()
    {
        static_assert(0 < sizeof(T), "can't delete an incomplete type");
        if (ptr && --(*refCount) == 0)
        {
            delete ptr;
            delete refCount;
        }
    }

    inline void IncrementRefCount() const
    {
        if (ptr)
        {
            ++*refCount;
        }
    }

public:
    Ref() = default;

    Ref(T* ptr)
        : ptr(ptr)
    {
        refCount = new count_t{ 0 };
        IncrementRefCount();
    }

    Ref(const Ref& other)
        : ptr(other.ptr)
        , refCount(other.refCount)
    {
        IncrementRefCount();
    }

    Ref(Ref&& other) noexcept
        : ptr(other.ptr)
        , refCount(other.refCount)
    {
        other.ptr = nullptr;
        other.refCount = nullptr;
    }

    template <class T2>
        requires std::is_base_of_v<T, T2> || std::is_base_of_v<T2, T>
    Ref(Ref<T2>& other)
        : ptr(other.Get())
        , refCount(other.RefCountPtr())
    {
        IncrementRefCount();
    }
    
    template <class T2>
        requires std::is_base_of_v<T, T2> || std::is_base_of_v<T2, T>
    Ref(Ref<T2>&& other)
        : ptr(other.Get())
        , refCount(other.RefCountPtr())
    {
        if (ptr)
        {
            IncrementRefCount();
            other.Reset();
        }
    }

    ~Ref()
    {
        DecrementRefCount();
    }

    Ref& operator=(T* ptr)
    {
        DecrementRefCount();
        this->ptr = ptr;
        refCount = ptr ? new count_t{ 1 } : nullptr;
        return *this;
    }

    Ref& operator=(const Ref& other)
    {
        DecrementRefCount();
        ptr = other.ptr;
        refCount = other.refCount;
        IncrementRefCount();
        return *this;
    }

    Ref& operator=(Ref&& other) noexcept
    {
        DecrementRefCount();
        ptr = other.ptr;
        refCount = other.refCount;
        other.ptr = nullptr;
        other.refCount = nullptr;
        return *this;
    }

    template <class T2>
        requires std::is_base_of_v<T, T2> || std::is_base_of_v<T2, T>
    Ref& operator=(Ref<T2>& other)
    {
        DecrementRefCount();
        ptr = other.Get();
        refCount = other.RefCountPtr();
        IncrementRefCount();
        return *this;
    }
    
    template <class T2>
        requires std::is_base_of_v<T, T2> || std::is_base_of_v<T2, T>
    Ref& operator=(Ref<T2>&& other)
    {
        IncrementRefCount();
        ptr = other.Get();
        refCount = other.RefCountPtr();
        other.Reset();
        return *this;
    }

    T* operator->() { return ptr; }
    const T* operator->() const { return ptr; }
    T& operator*() { return *ptr; }
    const T& operator*() const { return *ptr; }
    operator bool() const { return ptr != nullptr; }

    count_t* RefCountPtr() { return refCount; }
    size_t RefCount() const
    {
#ifdef SC_DEV_VERSION
        if (refCount == nullptr)
        {
            DebugPrint("Warning: attempting to fetch uninitialized refCount, returning nullptr.");
        }
#endif
        return refCount ? *refCount : refCount;
    }

    T* Get() { return ptr; }
    const T* Get() const { return ptr; }

    void Reset(T* ptr)
    {
        DecrementRefCount();
        this->ptr = ptr;
        refCount = ptr ? new uint32_t{ 1 } : nullptr;
    }

    void Reset()
    {
        DecrementRefCount();
        ptr = nullptr;
        refCount = nullptr;
    }

    void Swap(Ref& other)
    {
        std::swap(ptr, other.ptr);
        std::swap(refCount, other.refCount);
    }

    bool Valid() const
    {
        return ptr != nullptr;
    }

    template <class T> class WeakRef;
    explicit Ref(const WeakRef<T>& weak);
};


export template <class T, class... TArgs>
Ref<T> MakeRef(TArgs&&... args)
{
    return Ref(new T(std::forward<TArgs>(args)...));
}

export template <class T>
class WeakRef
{
    T* ptr = nullptr;
    using count_t = uint32_t;
    count_t* refCount = nullptr;
    
public:
    WeakRef() = default;
    
    // Constructor from Ref
    WeakRef(const Ref<T>& ref)
        : ptr(ref.Get())
        , refCount(ref.RefCountPtr())
    {
    }
    
    // Copy constructor
    WeakRef(const WeakRef& other)
        : ptr(other.ptr)
        , refCount(other.refCount)
    {
    }
    
    // Move constructor
    WeakRef(WeakRef&& other) noexcept
        : ptr(other.ptr)
        , refCount(other.refCount)
    {
        other.ptr = nullptr;
        other.refCount = nullptr;
    }
    
    // Assignment from Ref
    WeakRef& operator=(const Ref<T>& ref)
    {
        ptr = ref.Get();
        refCount = ref.RefCountPtr();
        return *this;
    }
    
    // Copy assignment
    WeakRef& operator=(const WeakRef& other)
    {
        ptr = other.ptr;
        refCount = other.refCount;
        return *this;
    }
    
    // Move assignment
    WeakRef& operator=(WeakRef&& other) noexcept
    {
        ptr = other.ptr;
        refCount = other.refCount;
        other.ptr = nullptr;
        other.refCount = nullptr;
        return *this;
    }
    
    // Check if the referenced object is still alive
    bool Valid() const
    {
        return ptr && refCount && *refCount > 0;
    }
    
    // Convert weak reference to strong reference
    Ref<T> Lock() const
    {
        if (!Valid())
            return Ref<T>();
            
        return Ref<T>(*this);
    }
    
    // Reset the weak reference
    void Reset()
    {
        ptr = nullptr;
        refCount = nullptr;
    }
    
    // Comparison operators
    bool operator==(const WeakRef& other) const
    {
        return ptr == other.ptr;
    }
    
    bool operator!=(const WeakRef& other) const
    {
        return !(*this == other);
    }
    
    // Helper functions
    T* Get() const { return ptr; }
    count_t* RefCountPtr() const { return refCount; }
};

export template <class T>
Ref<T>::Ref(const WeakRef<T>& weak)
: ptr(weak.Get())
        , refCount(weak.RefCountPtr())
{
    if (ptr && refCount && *refCount > 0) {
        IncrementRefCount();
    } else {
        ptr = nullptr;
        refCount = nullptr;
    }
}