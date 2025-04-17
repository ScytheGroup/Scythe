module;
#include <nfd.hpp>
module Common:FileBrowser;

import :FileBrowser;
import Common;

using namespace std::literals;

namespace FileBrowser
{

// Returns the chosen path
std::optional<std::filesystem::path> BrowseForFile(const std::filesystem::path& defaultPath, FilterListType fileFilters)
{
    NFD_Init();

    nfdu8char_t* outPath = nullptr;
    Array<nfdu8filteritem_t> filters = fileFilters;
    
    nfdopendialogu8args_t args{};
    args.filterList = filters.data();
    args.filterCount = static_cast<nfdfiltersize_t>(filters.size());
    std::string defaultPathStr = defaultPath.string();
    args.defaultPath = defaultPathStr.c_str();
    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);

    std::filesystem::path s{outPath ? outPath : ""};
    if (outPath)
        NFD_FreePathU8(outPath);

    const char* error = NFD_GetError();
    NFD_Quit();

    if (result != NFD_OKAY)
    {
        if (error)
            DebugPrintError("Error: {}", error);
        return {};
    }
    
    return s;
}

std::optional<std::filesystem::path> PickFolder(const std::filesystem::path& defaultPath)
{
    NFD_Init();

    nfdu8char_t* outPath = nullptr;
    std::string defaultPathStr = defaultPath.string();
    nfdresult_t result = NFD_PickFolder(&outPath, defaultPathStr.c_str());

    std::filesystem::path s{outPath ? outPath : ""};
    if (outPath)
        NFD_FreePathU8(outPath);

    const char* error = NFD_GetError();
    NFD_Quit();

    if (result != NFD_OKAY)
    {
        if (error)
            DebugPrintError("Error: {}", error);
        return {};
    }
    
    return s;
}

std::optional<std::filesystem::path> PickSaveLocation(const std::string& defaultFileName, const std::filesystem::path& defaultPath)
{
    NFD_Init();

    nfdu8char_t* outPath = nullptr;
    nfdsavedialogu8args_t args{};
    std::string defaultPathStr = defaultPath.string(); 
    args.defaultPath = defaultPathStr.c_str();
    args.defaultName = defaultFileName.c_str();
    nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);

    std::filesystem::path s{outPath ? outPath : ""};
    if (outPath)
        NFD_FreePathU8(outPath);

    const char* error = NFD_GetError();
    NFD_Quit();

    if (result != NFD_OKAY)
    {
        if (error)
            DebugPrintError("Error: {}", error);
        return {};
    }
    
    return s;
}

std::future<std::optional<std::filesystem::path>> PickFolerAsync(const std::filesystem::path& defaultPath)
{
    return std::async(std::launch::async, PickFolder, defaultPath);
}

std::future<std::optional<std::filesystem::path>> BrowseForFileAsync(const std::filesystem::path& defaultPath, FilterListType fileFilters)
{
    return std::async(std::launch::async, BrowseForFile, defaultPath, fileFilters);
};

}
