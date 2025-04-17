module;
#include "windows.h"
#include <ThirdParty/renderdoc/renderdoc_app.h>
module Graphics:RenderDoc;

#ifdef RENDERDOC
// This is for intellisense :(

import Common;

RenderDoc::RenderDoc(HWND handle) 
    : handle{handle}
{
    Initialize();

    if (renderDocApi)
    {
        DebugPrint("RenderDocAPI correctly loaded.");

        // By default RenderDoc hides all d3d11 debug errors, this reenables showing them
        renderDocApi->SetCaptureOptionU32(eRENDERDOC_Option_APIValidation, 1);
        renderDocApi->SetCaptureOptionU32(eRENDERDOC_Option_DebugOutputMute, 0);

        // Hide overlay initially because it is distracting
        ToggleOverlay(false);
        DisableDefaultCaptureKeys();
    }
    else
    {
        DebugPrint("RenderDocAPI was not able to be loaded. No captures will be available.");
    }
}

RenderDoc::~RenderDoc()
{
    FreeLibrary(renderDocDllModule); 
}

void RenderDoc::StartCapture() 
{
    if (isCurrentlyCapturing)
    {
        DebugPrint("Cannot start capture when already capturing...");
        return;
    }

    Assert(renderDocApi != nullptr, "RenderDoc API must be defined in order to start a capture.");
    renderDocApi->StartFrameCapture(nullptr, handle);
    isCurrentlyCapturing = true;
}

void RenderDoc::EndCapture(Device& device) 
{
    if (!isCurrentlyCapturing)
    {
        DebugPrint("Cannot end capture without starting one before...");
        return;
    }

    Assert(renderDocApi != nullptr, "RenderDoc API must be defined in order to end a capture.");
    bool succeeded = renderDocApi->EndFrameCapture(nullptr, handle);
    if (succeeded) 
    {
        VerifyValidRDInstance(device, true);
        isCurrentlyCapturing = false;
    }

    DebugPrint("Capture {}.", succeeded ? "succeeded" : "failed");
}

void RenderDoc::UpdateEndFrame(Device& device)
{
    if (isCurrentlyCapturing)
    {
        EndCapture(device);
    }
}

void RenderDoc::PerformCapture(Device& device, bool isDestroyCapture)
{
    if (!renderDocApi)
    {
        DebugPrint("RenderDoc capture called when the API was not correctly initialized...");
        return;
    }

    VerifyValidRDInstance(device, isDestroyCapture);

    renderDocApi->TriggerCapture();
    DebugPrint("Triggered RenderDoc capture!");
}

void RenderDoc::Initialize()
{
    // Attempt to get api if already injected!
    renderDocApi = TryGetRenderDocApi();

    if (renderDocApi)
        return;

 #ifdef _WIN32
    // Search for open RenderDoc instances
    std::vector<uint32_t> foundPIDs = GetAllProcessIdsWithName(L"qrenderdoc.exe");
    if (!foundPIDs.empty())
    {
        for (uint32_t pid : foundPIDs)
        {
            std::optional<HANDLE> handle = OpenWindowsProcess(pid);

            if (handle.has_value())
            {
                std::optional<std::string> processPath = GetWindowsProcessName(handle.value());

                if (processPath.has_value())
                {
                    if (ImportRenderDocApiFromRenderDocExePath(processPath.value()))
                    {
                        // And again RenderDoc's api limits what we can do :(
                        // No way to connect to already open app so we have no choice but to launch a new instance later on when capturing
                        // This means that at this point we're unable to do anything more, so break
                        CloseHandle(handle.value());
                        break;
                    }
                }

                CloseHandle(handle.value());
            }
        }
    }
#endif

    // Could search for the renderdoc in engine config (for example you set it on first engine start or manually in a config file)
    // TODO: Since we don't have any config code for now, we skip this.

#ifdef _WIN32
    // Search for RenderDoc installation using Windows system registry (only works if RenderDoc was installed normally, portable installations will not appear here and thus must pass through the previous init method)
    if (!renderDocApi)
    {
        std::optional<std::string> renderDocExePath = GetWindowsRegistryValueAsString(
            RenderDocWindowsRegistryKey, HKEY_LOCAL_MACHINE);

        if (renderDocExePath.has_value())
        {
            ImportRenderDocApiFromRenderDocExePath(renderDocExePath.value());
        }
    }
#endif

#ifdef __linux__
    // TODO: Search for RenderDoc installation on Linux and if found, load its .SO file
    if (!renderDocApi)
    {
        DebugPrint("Linux Render Doc initialization not defined. RenderDoc will not be available in the engine.");
    }
#endif
}

void RenderDoc::VerifyValidRDInstance(Device& device, bool isDestroyCapture) 
{
    if (isDestroyCapture)
    {
        DestroyAllOpenIrrelevantRenderDocInstances();
    }

    ToggleOverlay(true);

    if (renderDocLaunchedPID == 0)
    {
        OpenNewRenderDocUI();
    }
    else
    {
        ShowCurrentRenderDocUI();
    }

    if (renderDocApi->IsTargetControlConnected() != 1)
    {
        DebugPrint("Setting current window as active.");
        renderDocApi->SetActiveWindow(device.device.Get(), handle);
    }
}

void RenderDoc::DestroyAllOpenIrrelevantRenderDocInstances()
{
    // Search for open RenderDoc instances
    std::vector<uint32_t> foundPIDs = GetAllProcessIdsWithName(L"qrenderdoc.exe");
    if (!foundPIDs.empty())
    {
        for (uint32_t pid : foundPIDs)
        {
            // If currently connected, don't close the instance...
            if (pid == renderDocLaunchedPID)
                continue;

            if (std::optional<HANDLE> handle = OpenWindowsProcess(pid); handle.has_value())
            {
                TerminateProcess(handle.value(), 0);
                CloseHandle(handle.value());
            }
        }
    }
}

RENDERDOC_API_1_6_0* RenderDoc::TryGetRenderDocApi()
{
    RENDERDOC_API_1_6_0* renderDocApi = nullptr;

    // GetModuleHandleA's msdn doc mentions that we do not have to call FreeLibrary on it
    HMODULE renderDocDllModule = GetModuleHandleA("renderdoc.dll");
    if (renderDocDllModule == nullptr)
    {
        return nullptr;
    }

    if (pRENDERDOC_GetAPI getApi = ExtractGetApiFromHModule(renderDocDllModule); !getApi || getApi(eRENDERDOC_API_Version_1_6_0, reinterpret_cast<void**>(&renderDocApi)) != 1)
    {
        return nullptr;
    }

    return renderDocApi;
}

pRENDERDOC_GetAPI RenderDoc::ExtractGetApiFromHModule(HMODULE hmodule)
{
    pRENDERDOC_GetAPI getApi = nullptr;

    getApi = reinterpret_cast<pRENDERDOC_GetAPI>(GetProcAddress(hmodule, "RENDERDOC_GetAPI"));
    if (getApi == nullptr)
    {
        return nullptr;
    }

    return getApi;
}

bool RenderDoc::OpenNewRenderDocUI()
{
    // Open new instance of render doc
    renderDocLaunchedPID = renderDocApi->LaunchReplayUI(1, "");
    if (renderDocLaunchedPID == 0)
    {
        DebugPrint("Unable to launch new RenderDoc UI.");
        return false;
    }

    DebugPrint("Launched new RenderDoc UI!");
    return true;
}

void RenderDoc::ShowCurrentRenderDocUI()
{
    // Show already open instance of render doc
    if (renderDocApi->ShowReplayUI() == 0)
    {
        // Couldn't open instance, check with the pid that there is still a process with this name
        std::optional<HANDLE> handle = OpenWindowsProcess(renderDocLaunchedPID);

        if (handle.has_value())
        {
            auto procName = GetWindowsProcessName(handle.value());
            if (procName && !procName->ends_with("qrenderdoc.exe"))
            {
                // Invalid process, reset PID and launch new window
                renderDocLaunchedPID = 0;
            }
            else
            {
                // And heeeere RenderDoc's api limits what we can do :(
                // No way to connect to already open app so we have no choice but to launch a new instance :'(((
            }

            CloseHandle(handle.value());
        }

        DebugPrint("Unable to show currently open RenderDoc UI. Attempting to open new window instead.");
        if (!OpenNewRenderDocUI())
            return;
    }
    DebugPrint("Showing currently open RenderDoc UI!");
}

void RenderDoc::ToggleOverlay(bool isShown)
{
    renderDocApi->MaskOverlayBits(eRENDERDOC_Overlay_None, isShown ? eRENDERDOC_Overlay_All : eRENDERDOC_Overlay_None);
}

void RenderDoc::DisableDefaultCaptureKeys()
{
    renderDocApi->SetCaptureKeys(nullptr, 0);
}

bool RenderDoc::ImportRenderDocApiFromRenderDocExePath(const std::string& renderDocExePath)
{
    std::filesystem::path renderDocPath{ renderDocExePath };
    renderDocPath.remove_filename();
    renderDocPath.append("renderdoc.dll");

    // At this point we should have a valid url to the renderdoc.dll!
    try
    {
        renderDocDllModule = LoadLibraryW(renderDocPath.c_str());
    }
    catch (...)
    {
        DebugPrint("RenderDocAPI initialisation failed: Error Code {}", GetLastError());
        return false;
    }

    pRENDERDOC_GetAPI getApi = ExtractGetApiFromHModule(renderDocDllModule);
    getApi(TargetedRenderDocVersion, reinterpret_cast<void**>(&renderDocApi));
    return renderDocApi != nullptr;
}
#endif
