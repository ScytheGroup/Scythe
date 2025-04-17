export module Graphics:ResourceRegister;

import Common;

// Is there even a point(haha) to saving punctual lights?
// Probably not! Buuuut for resources that are more expensive to generate (like textures and buffers!) it would be a good idea to cache the resource as following:

// First register it in your system's initialisation
// punctualLightComponent->graphicsHandle = graphicsResources.RegisterGraphicsResource(PunctualLightData{});
        
// Then use it whenever needed in the following way:
// PunctualLightData& lightData = graphicsResources.GetGraphicsResource<PunctualLightData>(punctualLightComponent->graphicsHandle);
        
// Remember to destroy when the component dies, the lifecycle of this graphics resource is linked to the component!
// graphicsResources.ReleaseGraphicsResource(punctualLightComponent->graphicsHandle);
// This should be linked to the ECS as a component dies via the ECS. Could be done through many different ways but probably the function to destroy would be templated depending on the component's type.
// It would then have two implementations depending on if the type has a graphicsHandle field or not.
        
// I have only implemented the base of such a system as we currently do not have a need for this.
// One of many use cases for this could be for cloud volumes. We could select a noise texture in the imgui details panel of the volume which would then be imported and stored in the CloudVolumeComponent's graphics resource!
// Or even for GI probe volumes to store their probe data!
        
// Punctual Lights end up being a POD structure, its a better idea to just create and destroy each frame!

struct GraphicsResource
{
    virtual ~GraphicsResource() = default;
};

template<typename T>
struct GraphicsResourceWrapper : GraphicsResource
{
    explicit GraphicsResourceWrapper(T&& inValue)
        : value{ std::move(inValue) }
    {}
    ~GraphicsResourceWrapper() override {}

    T value;
};

export class ResourceRegister
{
public:
    ResourceRegister();

    template <typename T>
    Handle RegisterGraphicsResource(T&& value);

    template <typename T>
    T& GetGraphicsResource(Handle handle);

    void ReleaseGraphicsResource(Handle handle);

private:
    static constexpr uint32_t InitialResourcesSize = 32;

    std::vector<std::unique_ptr<GraphicsResource>> resources;
    std::vector<uint32_t> freeIndices;
};

template <typename T>
Handle ResourceRegister::RegisterGraphicsResource(T&& value)
{
    GraphicsResourceWrapper<T>* resourceWrapper = std::make_unique<GraphicsResourceWrapper<T>>(std::move(value));

    uint32_t registerHandle;
    if (freeIndices.empty())
    {
        registerHandle = resources.size();
        resources.push_back(std::move(resourceWrapper));
    }
    else
    {
        registerHandle = freeIndices.front();
        freeIndices = { freeIndices.begin() + 1, freeIndices.end() };
        resources[registerHandle] = std::move(resourceWrapper);
    }

    return registerHandle;
}

template <typename T>
T& ResourceRegister::GetGraphicsResource(Handle handle)
{
    if (handle >= resources.size() || resources[handle] == nullptr)
    {
        DebugPrintError("Cannot return handle of invalid resource.");
    }

    return *static_cast<GraphicsResourceWrapper<T>*>(resources[handle]).value;
}