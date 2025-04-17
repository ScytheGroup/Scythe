#pragma once

#ifdef PROFILING

import Profiling;

// Helper macros
#define CONCATENATE_DIRECT(s1, s2) s1##s2
#define SC_UNIQUE_NAME(n) CONCATENATE_DIRECT(n, __COUNTER__)

// Initialisation
#define SC_INIT_START()                                                                                                \
    {                                                                                                                  \
        auto& p = Profiler::Get();                                                                                     \
        p.StartInit();                                                                                                 \
    }

#define SC_INIT_END()                                                                                                  \
    {                                                                                                                  \
        auto& p = Profiler::Get();                                                                                     \
        p.EndInit();                                                                                                   \
    }

// Frames
#define SC_SCOPED_FRAME() ScopedFrame SC_UNIQUE_NAME(SC_frame){}

// CPU Profiling Events
#define SC_SCOPED_EVENT(name) CPUScope SC_UNIQUE_NAME(SC_cpuScope)(name)
#define SC_SCOPED_EVENT_FUNC() CPUScope SC_UNIQUE_NAME(SC_cpuScope)(__func__)

// GPU Profiling Events
// See https://stackoverflow.com/a/68007618 for (L"" name) at end of line
#define SC_SCOPED_GPU_EVENT(name) GPUScope SC_UNIQUE_NAME(SC_gpuScope)(L"" name)
#define SC_SCOPED_GPU_EVENT_FUNC() GPUScope SC_UNIQUE_NAME(SC_gpuScope)(__func__)
#define SC_SCOPED_GPU_EVENT_DYN_NAME(name) GPUScope SC_UNIQUE_NAME(SC_gpuScope)(name)

#else
#define SC_INIT_START()
#define SC_INIT_END()
#define SC_SCOPED_FRAME()
#define SC_SCOPED_EVENT(name)
#define SC_SCOPED_EVENT_FUNC()
#define SC_SCOPED_GPU_EVENT(name)
#define SC_SCOPED_GPU_EVENT_FUNC()
#define SC_SCOPED_GPU_EVENT_DYN_NAME(name)
#endif