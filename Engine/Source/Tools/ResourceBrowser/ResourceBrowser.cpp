module ResourceBrowser;
import <set>;

namespace fs = std::filesystem;

using namespace std::literals;

// Takes a system path and converts it to a ResourceBrowser path.
fs::path ResourceBrowser::FormatPath(const fs::path& path)
{    
    return fs::relative(path, ProjectSettings::Get().projectRoot);
}

Array<fs::path> ResourceBrowser::GetFilesWithExtension(std::set<std::string> extensions)
{
    // Walk through all files and subfiles and return all found files containing one of the extensions
    Array<fs::path> files;
    
    for (auto const& entry : fs::recursive_directory_iterator(ProjectSettings::Get().projectRoot / "Resources"))
    {
        if (entry.is_regular_file() && extensions.contains(entry.path().extension().string()))
            files.push_back(entry.path());
    }
    
    return files;
}

