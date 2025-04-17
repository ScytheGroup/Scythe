module Profiling:CPUScope;

import Profiling;
import :CPUScope;

CPUScope::CPUScope(std::string name, const std::source_location& location)
    : name{std::move(name)}
    , start{ std::chrono::high_resolution_clock::now() }
    , location{location}
{
    static Profiler& profiler = Profiler::Get();
    scopeLevel = profiler.PushNewCPUScopeLevel();
    reservedScopeIndex = profiler.ReserveCPUProfilerScopeIndex();
}

CPUScope::~CPUScope()
{
    ProfilerScope scope;
    scope.name = name;
    scope.start = start;
    scope.end = std::chrono::high_resolution_clock::now();
    scope.threadId = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    scope.functionName = location.function_name();
    scope.scopeLevel = scopeLevel;

    static Profiler& profiler = Profiler::Get();
    profiler.UpdateCPUProfilerScope(reservedScopeIndex, std::move(scope));
}