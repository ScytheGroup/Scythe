module;
#include "windows.h"
#include "Source/ThirdParty/renderdoc/renderdoc_app.h"
export module Graphics:RenderDoc;

#ifdef RENDERDOC
import :Device;

export class RenderDoc
{
public:
    RenderDoc(HWND handle);
    ~RenderDoc();

    void PerformCapture(Device& device, bool isDestroyCapture);

    void StartCapture();
    void EndCapture(Device& device);

    void UpdateEndFrame(Device& device);

private:
    void Initialize();

    void VerifyValidRDInstance(Device& device, bool isDestroyCapture);
    void DestroyAllOpenIrrelevantRenderDocInstances();

    static RENDERDOC_API_1_6_0* TryGetRenderDocApi();
    static pRENDERDOC_GetAPI ExtractGetApiFromHModule(HMODULE hmodule);

    bool OpenNewRenderDocUI();
    void ShowCurrentRenderDocUI();
    void ToggleOverlay(bool isShown);
    void DisableDefaultCaptureKeys();

    bool ImportRenderDocApiFromRenderDocExePath(const std::string& renderDocExePath);

    uint32_t renderDocLaunchedPID = 0;
    RENDERDOC_API_1_6_0* renderDocApi = nullptr;

    HMODULE renderDocDllModule{};

    HWND handle;

    bool isCurrentlyCapturing = false;

    static constexpr RENDERDOC_Version TargetedRenderDocVersion = eRENDERDOC_API_Version_1_6_0;
    // Default path of RenderDoc registry key, is used to attempt to get RenderDoc's installation path. Could fail, but this happens gracefully.
    static constexpr const char* RenderDocWindowsRegistryKey =
        "SOFTWARE\\Classes\\RenderDoc.RDCCapture.1\\DefaultIcon\\";
};

// Must be before device as to ensure that RenderDoc is able to register our CreateDevice call
export inline std::unique_ptr<RenderDoc> renderDoc;

#endif