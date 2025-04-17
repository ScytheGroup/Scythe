module Components:Component;

import Common;
import Core;

import :Component;

Component::Component(const Component& other)
{
    // We dont want to copy any of those data
}

void Component::LateInitialize(Handle newHandle, uint32_t newComponentId)
{
    entityHandle = newHandle;
    componentId = newComponentId;
}
