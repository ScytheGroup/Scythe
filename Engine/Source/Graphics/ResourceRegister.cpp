module Graphics:ResourceRegister;

import :ResourceRegister;

ResourceRegister::ResourceRegister()
{
    resources.reserve(InitialResourcesSize);
}

void ResourceRegister::ReleaseGraphicsResource(Handle handle)
{
    if (handle >= resources.size())
    {
        DebugPrintError("Unable to release handle that is lower than max registered resource.");
    }
    
    resources[handle].release();

    freeIndices.push_back(handle.id);
    std::ranges::sort(freeIndices, [] (uint32_t a, uint32_t b){ return a < b; });
}
