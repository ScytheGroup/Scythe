export module Editor:ResourceBrowserWidget;
import ResourceBrowser;

import Common;

export class ResourceBrowserWidget
{
    const std::filesystem::path basePath = ProjectSettings::Get().projectRoot;
    std::filesystem::path currentPath = basePath;

    void DrawFolder(std::filesystem::path folderPath);
    void DrawFile(std::filesystem::path filePath);
    size_t internalCount = 0;

    std::unordered_map<std::string, bool> openedFolders;
public:
    void Draw(bool* opened);
    std::filesystem::path Selected() {return currentPath;}
};
