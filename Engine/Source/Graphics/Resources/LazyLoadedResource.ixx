export module Graphics:LazyLoadedResource;

import :Device;

export class ILazyLoadedResource
{
protected:
    bool loaded = false;
    bool shouldFlushDataOnLoad = false;
public:
    bool IsLoaded() const noexcept { return loaded; }

    // Make sure to only call this when the object is at a fix spot in memory
    void RegisterLazyLoadedResource(bool shouldFlushData);
    
    virtual void LoadResource(Device& device);
    virtual ~ILazyLoadedResource();
};

