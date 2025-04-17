export module Common:Singleton;

import :Debug;

export template <class T> class StaticSingleton
{
public:
    static T& GetInstance()
    {
        return Instance;
    }

protected:

    StaticSingleton(void) {}
    ~StaticSingleton(void) {}

private:
    static T Instance; // Instance de la classe

    StaticSingleton(StaticSingleton&) = delete;
    void operator =(StaticSingleton&) = delete;
};

export template <class T>
class InstancedSingleton
{
    static T* Instance;
protected:
    explicit InstancedSingleton()
    {
        Assert(Instance == nullptr, "InstancedSingleton already instanciated");
        Instance = static_cast<T*>(this);
    }
    virtual ~InstancedSingleton()
    {
        Instance = nullptr;
    }
public:
    InstancedSingleton(const InstancedSingleton&) = delete;
    InstancedSingleton& operator=(const InstancedSingleton&) = delete;
    InstancedSingleton(InstancedSingleton&&) = delete;
    InstancedSingleton& operator=(InstancedSingleton&&) = delete;
    
    static T& Get()
    {
        Assert(Instance != nullptr, "Instance should be instanciated before get");
        return *Instance;
    }
    static T* GetPtr()
    {
        // We don't assert here because we want to be able to check if the instance is null
        return Instance;
    }
};
    
export template <class T> T StaticSingleton<T>::Instance;
export template <class T> T* InstancedSingleton<T>::Instance = nullptr;


