module;
#include <d3d11_1.h>
module Profiling:GPUScope;

import :GPUScope;
import Profiling;
import Graphics;

D3D11_QUERY_DESC queryTimestampDesc = { D3D11_QUERY::D3D11_QUERY_TIMESTAMP, 0 };

GPUScope::GPUScope(std::wstring eventName)
    : name{ std::move(eventName) }
{
    static Profiler& profiler = Profiler::Get();

    profiler.annotation->BeginEvent(name.c_str());

    chk << profiler.device.device->CreateQuery(&queryTimestampDesc, beginTimestampQuery.GetAddressOf());
    chk << profiler.device.device->CreateQuery(&queryTimestampDesc, endTimestampQuery.GetAddressOf());

    profiler.device.deviceContext->End(beginTimestampQuery.Get());
    scopeLevel = profiler.PushNewGPUScopeLevel();
    reservedScopeIndex = profiler.ReserveGPUProfilerScopeIndex();
}

GPUScope::GPUScope(const std::string& eventName)
    : GPUScope(std::move(StringToWString(eventName)))
{}

GPUScope::~GPUScope()
{
    static Profiler& profiler = Profiler::Get();
    profiler.device.deviceContext->End(endTimestampQuery.Get());
    profiler.annotation->EndEvent();

    RawGPUScope gpuScope;
    gpuScope.name = name;
    gpuScope.beginTimestampQuery = beginTimestampQuery;
    gpuScope.endTimestampQuery = endTimestampQuery;
    gpuScope.scopeLevel = scopeLevel;
    profiler.UpdateGPUProfilerScope(reservedScopeIndex, std::move(gpuScope));
}