module;
#include "ImGui/Icons/IconsFontAwesome6.h"
module Editor;
import ImGui;

import :ResourceBrowserWidget;

using namespace std::filesystem;
using namespace std::literals;

void ResourceBrowserWidget::Draw(bool* opened)
{
    if (ImGui::Begin("Resource Browser", opened))
    {
        internalCount = 0;
    
        if (ImGui::TreeNodeEx("Asset Browser #", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow, ICON_FA_FOLDER_OPEN" Assets")) // Folder icon
        {
            if (ImGui::IsItemClicked())
                currentPath = ProjectSettings::Get().projectRoot;
    
            DrawFolder(basePath / "Resources");
            DrawFolder(basePath / "Source");
    
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

void ResourceBrowserWidget::DrawFolder(path folderPath)
{
    {
        std::string directoryName = folderPath.filename().string();

        bool& opened = openedFolders[directoryName];
        auto icon = opened ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER;
        auto treename = icon + " "s + directoryName; // Replace [Dir] by an icon

        int treeflags = ImGuiTreeNodeFlags_OpenOnArrow;
        if (currentPath == folderPath)
            treeflags |= ImGuiTreeNodeFlags_Selected;

        opened = ImGui::TreeNodeEx(directoryName.c_str(), treeflags, treename.c_str());

        if (ImGui::IsItemClicked())
            currentPath = folderPath;

        if (!opened)
            return;
    }

    Array<path> dirs;
    Array<path> files;

    for (auto it : directory_iterator(folderPath))
    {
        it.is_directory() ? dirs.push_back(it) : files.push_back(it);
    }

    for (auto f : files)
        DrawFile(f);

    for (auto d : dirs)
        DrawFolder(d);


    ImGui::TreePop();
}

void ResourceBrowserWidget::DrawFile(path filePath)
{
    std::string directoryName = filePath.filename().string();
    auto treename = ICON_FA_FILE + " "s + directoryName; // Replace [File] by an icon

    int treeflags = ImGuiTreeNodeFlags_Leaf;
    if (currentPath == filePath)
        treeflags |= ImGuiTreeNodeFlags_Selected;

    if (ImGui::TreeNodeEx(directoryName.c_str(), treeflags, treename.c_str()))
    {
        if (ImGui::IsItemClicked())
            currentPath = filePath;
        ImGui::TreePop();
    }
}
