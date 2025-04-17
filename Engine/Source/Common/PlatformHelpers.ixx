module;
#include "windows.h"
#include "tlhelp32.h"
export module Common:PlatformHelpers;

import :Debug;
import <string>;
import <optional>;
import <vector>;

#ifdef _WIN32

export inline std::optional<std::string> GetWindowsRegistryValueAsString(const char* registryKey, HKEY keyHandle)
{
    CHAR szBuffer[1024];
    DWORD dwBufferSize = sizeof(szBuffer);
    ULONG nError = RegGetValueA(keyHandle, registryKey,
                                nullptr, RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND,
                                nullptr,
                                szBuffer,
                                &dwBufferSize);

    if (nError == ERROR_SUCCESS)
    {
        return std::string{ szBuffer };
    }

    DebugPrint("Getting registry value failed: {}, {}", registryKey, GetLastError());
    return std::nullopt;
}

export inline std::optional<std::string> GetWindowsProcessName(HANDLE processHandle)
{
    DWORD buffSize = 1024;
    CHAR buffer[1024];
    if (QueryFullProcessImageNameA(processHandle, 0, buffer, &buffSize))
    {
        return std::string{ buffer };
    }

    DebugPrint("Getting process name failed: {}", GetLastError());
    return std::nullopt;
}

export inline std::optional<HANDLE> OpenWindowsProcess(uint32_t pid)
{
    if (HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid); handle != nullptr)
    {
        return handle;
    }

    DebugPrint("Opening process failed with pid: {}, {}", pid, GetLastError());
    return std::nullopt;
}

export inline std::vector<uint32_t> GetAllProcessIdsWithName(const std::wstring& processName)
{
    PROCESSENTRY32 pe;

    // Snapshot of all processes in the system
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hSnapshot)
        return {};

    // Initializing size: needed for using Process32First
    pe.dwSize = sizeof(PROCESSENTRY32);

    // Info about first process encountered in a system snapshot
    BOOL hResult = Process32First(hSnapshot, &pe);

    std::vector<uint32_t> pids;

    // Retrieve information about the processes
    // and exit if unsuccessful
    while (hResult)
    {
        // if we find the process: return process ID
        if (processName == std::wstring(pe.szExeFile))
        {
            pids.push_back(pe.th32ProcessID);
        }
        hResult = Process32Next(hSnapshot, &pe);
    }

    // Closes an open handle (CreateToolhelp32Snapshot)
    CloseHandle(hSnapshot);
    return pids;
}

export std::string HRToError(HRESULT hr)
{
    wchar_t* descriptionWinalloc = nullptr;
    const auto result = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&descriptionWinalloc), 0, nullptr
    );

    if (!result)
    {
        DebugPrint("Failed to format windows error.");
        return "";
    }

    std::wstring description = descriptionWinalloc;
    if (LocalFree(descriptionWinalloc))
    {
        DebugPrint("Failed to free memory for windows error formatting.");
        return "";
    }

    return WStringToString(description);
}

#endif