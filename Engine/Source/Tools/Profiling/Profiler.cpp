module;
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "d3d11_1.h"
#include "Common/Macros.hpp"
module Profiling;

import Common;
import ImGui;
import Graphics;

Profiler* instance = nullptr;

Profiler::Profiler(Device& device)
    : device{ device }
{
    chk << device.deviceContext->QueryInterface(__uuidof(annotation.Get()),
                                                reinterpret_cast<void**>(annotation.GetAddressOf()));
    D3D11_QUERY_DESC queryTimestampDisjointDesc = { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };
    for (ComPtr<ID3D11Query>& queryPtr : disjointQueries)
    {
        chk << device.device->CreateQuery(&queryTimestampDisjointDesc, queryPtr.GetAddressOf());
    }
    instance = this;

    IMGUI_REGISTER_WINDOW("Profiling", TOOLS, Profiler::DrawCPU);
}

Profiler::~Profiler()
{
    instance = nullptr;
}

Profiler& Profiler::Get()
{
    Assert(instance != nullptr, "Profiler must have been initialized before calling Profiler::Get()");
    return *instance;
}

void Profiler::Update(float deltaTime)
{
    currentDisplayTime += deltaTime;
    if (currentDisplayTime > displayRefreshTime)
    {
        currentDisplayTime = 0;

        if (isCPUProfilingActive)
        {
            int cpuIndex = (cpuFrameIndex + 1) % CPUFrameLatency;
            stableCPUFrame = cpuFrame[cpuIndex];
            stableFrameCPUTime = std::chrono::duration<float, std::milli>(stableCPUFrame.end - stableCPUFrame.start).count();
        }

        if (isGPUProfilingActive)
        {
            int gpuIndex = (gpuFrameIndex + 1) % GPUFrameLatency;
            if (gpuFrame[gpuIndex].isValid)
            {
                stableGPUFrame = gpuFrame[gpuIndex];
                stableFrameGPUTime = stableGPUFrame.GetTimeFromInterval(stableGPUFrame.start, stableGPUFrame.end);
            }
        }
    }
}

void Profiler::StartInit()
{
    startupData.start = std::chrono::high_resolution_clock::now();
}

void Profiler::EndInit()
{
    startupData.end = std::chrono::high_resolution_clock::now();
    startupData.length = std::chrono::duration<float, std::milli>(startupData.end - startupData.start);
}

void Profiler::StartFrame()
{
    // CPU
    if (isCPUProfilingActive)
    {
        cpuFrame[cpuFrameIndex] = {};
        cpuFrame[cpuFrameIndex].start = std::chrono::high_resolution_clock::now();
        cpuScopeLevel = 0;
    }

    // GPU
    if (isGPUProfilingActive)
    {
        rawGPUFrames.push({});

        // Add frame delay to reduce waiting for query results
        if (rawGPUFrames.size() >= GPUFrameLatency)
            instance->device.deviceContext->Begin(disjointQueries[gpuFrameIndex].Get());

        gpuScopeLevel = 0;
    }
}

void Profiler::EndFrame()
{
    // CPU
    {
        // Can be empty if history was cleared this frame
        if (isCPUProfilingActive)
        {
            cpuFrame[cpuFrameIndex].end = std::chrono::high_resolution_clock::now();
            cpuFrameIndex = (cpuFrameIndex + 1) % CPUFrameLatency;
        }
    }

    // GPU
    {
        // Add frame delay to reduce waiting for query results
        if (!isGPUProfilingActive || rawGPUFrames.size() < GPUFrameLatency)
            return;

        // We only transform the values once the frame is over since GetData calls will wait for value to be available.
        instance->device.deviceContext->End(disjointQueries[gpuFrameIndex].Get());
        
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
        while (device.deviceContext->GetData(disjointQueries[gpuFrameIndex].Get(), &disjointData, sizeof(disjointData), 0) != S_OK) {}
        
        float frequency = static_cast<float>(disjointData.Frequency);

        gpuFrame[gpuFrameIndex] = {};
        gpuFrame[gpuFrameIndex].frequency = frequency;
        gpuFrame[gpuFrameIndex].isValid = !disjointData.Disjoint;
        gpuFrame[gpuFrameIndex].profilerScopes.reserve(rawGPUFrames.front().size());

        float totalTime = 0;
        for (RawGPUScope& rawGPUScope : rawGPUFrames.front())
        {
            if (!rawGPUScope.HasFinished()) continue;
            
            GPUProfilerScope& scope = gpuFrame[gpuFrameIndex].profilerScopes.emplace_back();
            scope.name = WStringToString(rawGPUScope.name);
            scope.scopeLevel = rawGPUScope.scopeLevel;
            while (device.deviceContext->GetData(rawGPUScope.beginTimestampQuery.Get(), &scope.start, sizeof(scope.start), 0) != S_OK) {}
            while (device.deviceContext->GetData(rawGPUScope.endTimestampQuery.Get(), &scope.end, sizeof(scope.end), 0) != S_OK) {}

            uint64_t delta = scope.end - scope.start;
            float time = delta / frequency * 1000.0f;
            totalTime += time;
        }
        rawGPUFrames.pop();

        gpuFrame[gpuFrameIndex].start = gpuFrame[gpuFrameIndex].profilerScopes.empty() ? 0 : gpuFrame[gpuFrameIndex].profilerScopes.front().start;
        gpuFrame[gpuFrameIndex].end = gpuFrame[gpuFrameIndex].profilerScopes.empty() ? 0 : gpuFrame[gpuFrameIndex].profilerScopes.back().end;
        gpuFrameIndex = (gpuFrameIndex + 1) % GPUFrameLatency;
    }
}

int Profiler::ReserveCPUProfilerScopeIndex()
{
    if (!isCPUProfilingActive) return 0;
    
    cpuFrame[cpuFrameIndex].profilerScopes.emplace_back();
    return static_cast<int>(cpuFrame[cpuFrameIndex].profilerScopes.size() - 1);
}

int Profiler::ReserveGPUProfilerScopeIndex()
{
    if (!isGPUProfilingActive || rawGPUFrames.empty()) return 0;
    
    rawGPUFrames.back().emplace_back();
    return static_cast<int>(rawGPUFrames.back().size() - 1);
}

void Profiler::UpdateCPUProfilerScope(int scopeIndex, ProfilerScope profilerScope)
{
    if (!isCPUProfilingActive) return;

    cpuScopeLevel--;
    cpuFrame[cpuFrameIndex].profilerScopes[scopeIndex] = std::move(profilerScope);
}

void Profiler::UpdateGPUProfilerScope(int scopeIndex, RawGPUScope gpuScope)
{
    if (!isGPUProfilingActive || rawGPUFrames.empty()) return;
    
    gpuScopeLevel--;
    rawGPUFrames.back()[scopeIndex] = std::move(gpuScope);
}

void Profiler::Draw()
{
    ImGui::Text("CPU Frame Time: %4.4gms", stableFrameCPUTime);
    ImGui::Text("FPS: %4.4gfps", 1000.0f / stableFrameCPUTime);
    ImGui::Text("GPU Frame Time: %4.4gms", stableFrameGPUTime);
    ImGui::Text("Engine Startup Time: %4.4gms", startupData.length.count());

    ImGui::NewLine();

    ImGui::Text("Display Refresh Time (in seconds):");
    ImGui::SliderFloat("##Display Refresh Time", &displayRefreshTime, 0, 4);

    ImGui::Separator();

    ImGui::Checkbox("Activate CPU profiling", &isCPUProfilingActive);
    ImGui::Checkbox("Activate GPU profiling", &isGPUProfilingActive);
    
    ImGui::NewLine();

    // CPU Details Panel
    {
        ImGui::SeparatorText("Details (CPU)");
        DrawCPUDetailsPanel();
    }

    // GPU Details Panel
    {
        ImGui::SeparatorText("Details (GPU)");
        DrawGPUDetailsPanel();
    }
}

void Profiler::DrawCPU(bool* opened)
{
    if (ImGui::Begin("Profiling", opened))
    {
        Draw();
    }
    ImGui::End();
}

void Profiler::DrawCPUDetailsPanel()
{
    ImGui::Text("Total Frame Time: %4.4gms", stableFrameCPUTime);

    for (ProfilerScope& profilerScope : stableCPUFrame.profilerScopes)
    {
        for (int i = 0; i < profilerScope.scopeLevel; ++i)
            ImGui::Indent();

        float scopeTime = std::chrono::duration<float, std::milli>(profilerScope.end - profilerScope.start).count();
        ImGui::Text("%s: %4.4gms", profilerScope.name.c_str(), scopeTime);

        for (int i = 0; i < profilerScope.scopeLevel; ++i)
            ImGui::Unindent();
    }
}

void Profiler::DrawGPUDetailsPanel()
{
    if (!stableGPUFrame.isValid)
    {
        ImGui::TextColored({ 1, 0, 0, 1 }, "No GPU details yet...");
        return;
    }
    
    ImGui::Text("Total GPU Frame Time: %4.4gms", stableFrameGPUTime);

    for (GPUProfilerScope& profilerScope : stableGPUFrame.profilerScopes)
    {
        for (int i = 0; i < profilerScope.scopeLevel; ++i)
            ImGui::Indent();
        float scopeTime = stableGPUFrame.GetTimeFromInterval(profilerScope.start, profilerScope.end);

        ImGui::Text("%s: %4.4gms", profilerScope.name.c_str(), scopeTime);
        for (int i = 0; i < profilerScope.scopeLevel; ++i)
            ImGui::Unindent();
    }
}
