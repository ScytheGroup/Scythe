module;

#define DEBUG_LAZY_LOADING 0
#if DEBUG_LAZY_LOADING
#include <stacktrace>
#endif

module Graphics:LazyLoadedResource;

import :LazyLoadedResource;
import :ResourceManager;

void ILazyLoadedResource::RegisterLazyLoadedResource(bool shouldFlushData)
{
    shouldFlushDataOnLoad = shouldFlushData;
    if (!loaded)
    {
        std::unique_lock lock{ResourceManager::Get().lazyLoadMutex};
        ResourceManager::Get().resourcesToLazyLoad.insert(this);

#if DEBUG_LAZY_LOADING // This allows for tracing the calls to lazy loading. Helps greatly for debugging but spams the logs
        DebugPrint("Registered Resource: {:#010x}\nTrace:\n{}", reinterpret_cast<intptr_t>(this), std::stacktrace::current(0, 30));
#endif
    }
}

void ILazyLoadedResource::LoadResource(Device&)
{
    loaded = true;
    std::unique_lock lock{ResourceManager::Get().lazyLoadMutex};
    ResourceManager::Get().resourcesToLazyLoad.erase(this);
}

ILazyLoadedResource::~ILazyLoadedResource()
{
    // Will return false if we are shutting down
    if (auto manager = ResourceManager::GetPtr())
    {
#if DEBUG_LAZY_LOADING
        DebugPrint("Unregistered Resource: {:#010x}", reinterpret_cast<intptr_t>(this));
#endif
        std::unique_lock lock{ResourceManager::Get().lazyLoadMutex};
        manager->resourcesToLazyLoad.erase(this);
    }
        
}
