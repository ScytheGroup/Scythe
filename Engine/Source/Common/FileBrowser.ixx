module;
#include "nfd.hpp"

export module Common:FileBrowser;

import :Array;
import :ProjectSettings;

import <future>;
import <optional>;

export namespace FileBrowser
{
    using FilterListType = Array<nfdu8filteritem_t>;
    std::optional<std::filesystem::path> BrowseForFile(const std::filesystem::path& relativePathFromRoot = ProjectSettings::Get().projectRoot, FilterListType fileFilters = {});
    std::optional<std::filesystem::path> PickFolder(const std::filesystem::path& relativePathFromRoot = ProjectSettings::Get().projectRoot);
    std::optional<std::filesystem::path> PickSaveLocation(const std::string& defaultFileName = "NewFile", const std::filesystem::path& projectRelativeDefaultPath = ProjectSettings::Get().projectRoot);
    std::future<std::optional<std::filesystem::path>> BrowseForFileAsync(const std::filesystem::path& relativePathFromRoot = ProjectSettings::Get().projectRoot, FilterListType fileFilters = {});
    std::future<std::optional<std::filesystem::path>> PickFolerAsync(const std::filesystem::path& relativePathFromRoot = ProjectSettings::Get().projectRoot);
}

