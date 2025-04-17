module;
#include "AnnotationInclude.h"
export module Profiling;

export import :CPUScope;
export import :GPUScope;

import Common;

export class Device;

export struct ProfilerScope
{
    std::string name;
    std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    uint32_t threadId;
    std::string functionName;
    uint8_t scopeLevel;
};

struct ProfilerFrame
{
    std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    std::vector<ProfilerScope> profilerScopes;
};

struct GPUProfilerScope
{
    std::string name;
    uint64_t start, end;
    uint8_t scopeLevel;
};

struct GPUProfilerFrame
{
    uint64_t start, end;
    float frequency;
    bool isValid = false;
    std::vector<GPUProfilerScope> profilerScopes;

    float GetTimeFromInterval(uint64_t intervalStart, uint64_t intervalEnd) const noexcept
    {
        uint64_t delta = intervalEnd - intervalStart;
        return delta / frequency * 1000.0f;
    }
};

export class Profiler
{
public:
    Profiler(Device& device);
    ~Profiler();

    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;
    Profiler(Profiler&&) = delete;
    Profiler& operator=(Profiler&&) = delete;

    static Profiler& Get();

    void Update(float deltaTime);
    
    void StartInit();
    void EndInit();

    void StartFrame();
    void EndFrame();
    int ReserveCPUProfilerScopeIndex();
    int ReserveGPUProfilerScopeIndex();
    void UpdateCPUProfilerScope(int scopeIndex, ProfilerScope profilerScope);
    void UpdateGPUProfilerScope(int scopeIndex, RawGPUScope gpuScope);
    void Draw();
    void DrawCPU(bool* opened);

    uint8_t PushNewCPUScopeLevel() noexcept
    {
        return cpuScopeLevel++;
    }

    uint8_t PushNewGPUScopeLevel() noexcept
    {
        return gpuScopeLevel++;
    }

private:
    void DrawCPUDetailsPanel();
    void DrawGPUDetailsPanel();

    bool isCPUProfilingActive = true;
    bool isGPUProfilingActive = false;

    static constexpr int CPUFrameLatency = 3;
    int cpuFrameIndex = 0;
    
    ProfilerFrame cpuFrame[CPUFrameLatency];
    uint8_t cpuScopeLevel = 0;

    ComPtr<ID3DUserDefinedAnnotation> annotation;
    Device& device;

    static constexpr int GPUFrameLatency = 5;
    int gpuFrameIndex = 0;

    std::queue<std::vector<RawGPUScope>> rawGPUFrames;
    GPUProfilerFrame gpuFrame[GPUFrameLatency];
    ComPtr<ID3D11Query> disjointQueries[GPUFrameLatency];
    uint8_t gpuScopeLevel = 0;

    ProfilerFrame stableCPUFrame;
    float stableFrameCPUTime = 0;
    GPUProfilerFrame stableGPUFrame;
    float stableFrameGPUTime = 0;
    
    float displayRefreshTime = 0.3f;
    float currentDisplayTime = 0;
    
    struct StartupData
    {
        std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
        std::chrono::duration<float, std::milli> length;
    } startupData;

    friend struct GPUScope;
};

export struct ScopedFrame
{
    ScopedFrame()
    {
        static Profiler& p = Profiler::Get();
        p.StartFrame();
    }
    ~ScopedFrame()
    {
        static Profiler& p = Profiler::Get();
        p.EndFrame();
    }
};